/*
 * Copyright 2019 WolkAbout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "protocol/json/JsonGatewayStatusProtocol.h"
#include "model/DeviceStatus.h"
#include "model/Message.h"
#include "protocol/json/Json.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"

#include <algorithm>

using nlohmann::json;

namespace wolkabout
{
const std::string JsonGatewayStatusProtocol::LAST_WILL_TOPIC_ROOT = "lastwill/";
const std::string JsonGatewayStatusProtocol::DEVICE_STATUS_RESPONSE_TOPIC_ROOT = "d2p/subdevice_status_response/";
const std::string JsonGatewayStatusProtocol::DEVICE_STATUS_UPDATE_TOPIC_ROOT = "d2p/subdevice_status_update/";
const std::string JsonGatewayStatusProtocol::DEVICE_STATUS_REQUEST_TOPIC_ROOT = "p2d/subdevice_status_request/";

const std::string JsonGatewayStatusProtocol::STATUS_RESPONSE_STATE_FIELD = "state";
const std::string JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_CONNECTED = "CONNECTED";
const std::string JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_SLEEP = "SLEEP";
const std::string JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_SERVICE = "SERVICE";
const std::string JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_OFFLINE = "OFFLINE";

static void from_json(const json& j, DeviceStatus& p)
{
    const std::string statusStr = j.at(JsonGatewayStatusProtocol::STATUS_RESPONSE_STATE_FIELD).get<std::string>();

    const DeviceStatus::Status status = [&] {
        if (statusStr == JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_CONNECTED)
        {
            return DeviceStatus::Status::CONNECTED;
        }
        else if (statusStr == JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_SLEEP)
        {
            return DeviceStatus::Status::SLEEP;
        }
        else if (statusStr == JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_SERVICE)
        {
            return DeviceStatus::Status::SERVICE;
        }
        else if (statusStr == JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_OFFLINE)
        {
            return DeviceStatus::Status::OFFLINE;
        }

        throw std::logic_error("Invalid value for device status");
    }();
}

std::vector<std::string> JsonGatewayStatusProtocol::getInboundChannels() const
{
    return {DEVICE_STATUS_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            DEVICE_STATUS_UPDATE_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            LAST_WILL_TOPIC_ROOT + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewayStatusProtocol::getInboundChannelsForDevice(const std::string& deviceKey) const
{
    return {DEVICE_STATUS_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey,
            DEVICE_STATUS_UPDATE_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey,
            LAST_WILL_TOPIC_ROOT + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::unique_ptr<Message> JsonGatewayStatusProtocol::makeDeviceStatusRequestMessage(const std::string& deviceKey) const
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = DEVICE_STATUS_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey;

    return std::unique_ptr<Message>(new Message("", topic));
}

std::unique_ptr<DeviceStatus> JsonGatewayStatusProtocol::makeDeviceStatusResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    if (!StringUtils::startsWith(message.getChannel(), DEVICE_STATUS_RESPONSE_TOPIC_ROOT))
    {
        return nullptr;
    }

    try
    {
        const auto key = extractDeviceKeyFromChannel(message.getChannel());
        if (key.empty())
        {
            LOG(DEBUG) << "Gateway status protocol: Unable to extract device key: " << message.getChannel();
            return nullptr;
        }

        const json j = json::parse(message.getContent());
        DeviceStatus::Status state = j;

        return std::unique_ptr<DeviceStatus>(new DeviceStatus{key, state});
    }
    catch (const std::exception& e)
    {
        LOG(DEBUG) << "Gateway Status protocol: Unable to deserialize device status response: " << e.what();
        return nullptr;
    }
    catch (...)
    {
        LOG(DEBUG) << "Gateway Status protocol: Unable to deserialize device status response";
        return nullptr;
    }
}

std::unique_ptr<DeviceStatus> JsonGatewayStatusProtocol::makeDeviceStatusUpdate(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    if (!StringUtils::startsWith(message.getChannel(), DEVICE_STATUS_UPDATE_TOPIC_ROOT))
    {
        return nullptr;
    }

    try
    {
        const auto key = extractDeviceKeyFromChannel(message.getChannel());
        if (key.empty())
        {
            LOG(DEBUG) << "Gateway status protocol: Unable to extract device key: " << message.getChannel();
            return nullptr;
        }

        const json j = json::parse(message.getContent());
        DeviceStatus::Status state = j;

        return std::unique_ptr<DeviceStatus>(new DeviceStatus{key, state});
    }
    catch (const std::exception& e)
    {
        LOG(DEBUG) << "Gateway Status protocol: Unable to deserialize device status update: " << e.what();
        return nullptr;
    }
    catch (...)
    {
        LOG(DEBUG) << "Gateway Status protocol: Unable to deserialize device status update";
        return nullptr;
    }
}

bool JsonGatewayStatusProtocol::isStatusUpdateMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_STATUS_RESPONSE_TOPIC_ROOT);
}

bool JsonGatewayStatusProtocol::isStatusResponseMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_STATUS_RESPONSE_TOPIC_ROOT);
}

bool JsonGatewayStatusProtocol::isLastWillMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    std::string top = message.getChannel();

    if (!StringUtils::endsWith(top, CHANNEL_DELIMITER))
    {
        top.append(CHANNEL_DELIMITER);
    }

    return StringUtils::startsWith(top, LAST_WILL_TOPIC_ROOT);
}

std::string JsonGatewayStatusProtocol::routeDeviceMessage(const std::string& topic, const std::string& gatewayKey) const
{
    LOG(TRACE) << METHOD_INFO;

    const std::string deviceTopicPart = CHANNEL_DELIMITER + DEVICE_PATH_PREFIX;
    const std::string gatewayTopicPart = CHANNEL_DELIMITER + GATEWAY_PATH_PREFIX + gatewayKey;

    const auto position = topic.find(deviceTopicPart);
    if (position != std::string::npos)
    {
        std::string routedTopic = topic;
        return routedTopic.insert(position, gatewayTopicPart);
    }

    return "";
}

std::string JsonGatewayStatusProtocol::routePlatformMessage(const std::string& topic,
                                                            const std::string& gatewayKey) const
{
    LOG(TRACE) << METHOD_INFO;

    const std::string gwTopicPart = GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER;
    if (topic.find(gwTopicPart) != std::string::npos)
    {
        return StringUtils::removeSubstring(topic, gwTopicPart);
    }

    return "";
}

std::string JsonGatewayStatusProtocol::extractDeviceKeyFromChannel(const std::string& topic) const
{
    LOG(TRACE) << METHOD_INFO;

    std::string top = topic;
    if (StringUtils::endsWith(top, CHANNEL_DELIMITER))
    {
        top = top.substr(0, top.size() - CHANNEL_DELIMITER.size());
    }

    if (StringUtils::startsWith(top, LAST_WILL_TOPIC_ROOT))
    {
        auto delimiterPosition = top.find(CHANNEL_DELIMITER);
        if (delimiterPosition == std::string::npos)
        {
            return "";
        }

        return top.substr(delimiterPosition + CHANNEL_DELIMITER.size(), std::string::npos);
    }

    return GatewayProtocol::extractDeviceKeyFromChannel(top);
}

std::vector<std::string> JsonGatewayStatusProtocol::extractDeviceKeysFromContent(const std::string& content) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json j = json::parse(content);
        if (!j.is_array())
        {
            throw std::logic_error("");
        }

        std::vector<std::string> keys;

        for (const auto& key : j)
        {
            keys.push_back(key);
        }

        return keys;
    }
    catch (std::exception& e)
    {
        LOG(TRACE) << "Gateway status protocol: Unable extract file keys from content: " << e.what();
        return {};
    }
    catch (...)
    {
        LOG(TRACE) << "Gateway status protocol: Unable extract file keys from content";
        return {};
    }
}
}    // namespace wolkabout
