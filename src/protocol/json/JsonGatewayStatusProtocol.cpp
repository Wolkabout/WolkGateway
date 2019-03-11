/*
 * Copyright 2018 WolkAbout Technology s.r.o.
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
#include "model/DeviceStatusResponse.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"

#include <algorithm>

using nlohmann::json;

namespace wolkabout
{
const std::string JsonGatewayStatusProtocol::NAME = "StatusProtocol";

const std::string JsonGatewayStatusProtocol::CHANNEL_DELIMITER = "/";
const std::string JsonGatewayStatusProtocol::CHANNEL_MULTI_LEVEL_WILDCARD = "#";
const std::string JsonGatewayStatusProtocol::CHANNEL_SINGLE_LEVEL_WILDCARD = "+";
const std::string JsonGatewayStatusProtocol::GATEWAY_PATH_PREFIX = "g/";
const std::string JsonGatewayStatusProtocol::DEVICE_PATH_PREFIX = "d/";
const std::string JsonGatewayStatusProtocol::DEVICE_TO_PLATFORM_DIRECTION = "d2p/";
const std::string JsonGatewayStatusProtocol::PLATFORM_TO_DEVICE_DIRECTION = "p2d/";

const std::string JsonGatewayStatusProtocol::LAST_WILL_TOPIC_ROOT = "lastwill/";
const std::string JsonGatewayStatusProtocol::PLATFORM_STATUS_REQUEST_TOPIC_ROOT = "p2d/subdevice_status_request/";
const std::string JsonGatewayStatusProtocol::PLATFORM_STATUS_RESPONSE_TOPIC_ROOT = "d2p/subdevice_status_response/";
const std::string JsonGatewayStatusProtocol::PLATFORM_STATUS_CONFIRM_TOPIC_ROOT = "p2d/subdevice_status_confirm/";
const std::string JsonGatewayStatusProtocol::PLATFORM_STATUS_UPDATE_TOPIC_ROOT = "d2p/subdevice_status_update/";
const std::string JsonGatewayStatusProtocol::DEVICE_STATUS_REQUEST_TOPIC_ROOT = "p2d/status/";
const std::string JsonGatewayStatusProtocol::DEVICE_STATUS_RESPONSE_TOPIC_ROOT = "d2p/status/";
const std::string JsonGatewayStatusProtocol::PING_TOPIC_ROOT = "ping/";
const std::string JsonGatewayStatusProtocol::PONG_TOPIC_ROOT = "pong/";

const std::string JsonGatewayStatusProtocol::STATUS_RESPONSE_STATE_FIELD = "state";
const std::string JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_CONNECTED = "CONNECTED";
const std::string JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_SLEEP = "SLEEP";
const std::string JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_SERVICE = "SERVICE";
const std::string JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_OFFLINE = "OFFLINE";

static void to_json(json& j, const DeviceStatusResponse& p)
{
    const std::string status = [&]() -> std::string {
        switch (p.getStatus())
        {
        case DeviceStatus::CONNECTED:
        {
            return JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_CONNECTED;
        }
        case DeviceStatus::SLEEP:
        {
            return JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_SLEEP;
        }
        case DeviceStatus::SERVICE:
        {
            return JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_SERVICE;
        }
        case DeviceStatus::OFFLINE:
        default:
        {
            return JsonGatewayStatusProtocol::STATUS_RESPONSE_STATUS_OFFLINE;
        }
        }
    }();

    j = json{{JsonGatewayStatusProtocol::STATUS_RESPONSE_STATE_FIELD, status}};
}

static void to_json(json& j, const std::shared_ptr<DeviceStatusResponse>& p)
{
    if (!p)
    {
        return;
    }

    to_json(j, *p);
}

const std::string& JsonGatewayStatusProtocol::getName() const
{
    return NAME;
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

std::unique_ptr<Message> JsonGatewayStatusProtocol::makeMessage(const std::string& gatewayKey,
                                                                const std::string& deviceKey,
                                                                const DeviceStatusResponse& response) const
{
    LOG(TRACE) << METHOD_INFO;

    const json jPayload(response);
    const std::string topic = PLATFORM_STATUS_UPDATE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                              DEVICE_PATH_PREFIX + deviceKey;

    const std::string payload = jPayload.dump();

    return std::unique_ptr<Message>(new Message(payload, topic));
}

std::unique_ptr<Message> JsonGatewayStatusProtocol::makeDeviceStatusRequestMessage(const std::string& deviceKey) const
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = DEVICE_STATUS_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey;

    const std::string payload = "";

    return std::unique_ptr<Message>(new Message(payload, topic));
}

std::unique_ptr<Message> JsonGatewayStatusProtocol::makeFromPingRequest(const std::string& gatewayKey) const
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = PING_TOPIC_ROOT + gatewayKey;
    return std::unique_ptr<Message>(new Message("", topic));
}

std::unique_ptr<Message> JsonGatewayStatusProtocol::makeLastWillMessage(const std::string& gatewayKey) const
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = LAST_WILL_TOPIC_ROOT + gatewayKey;
    const std::string payload = "";

    return std::unique_ptr<Message>(new Message(payload, topic));
}

std::unique_ptr<DeviceStatusResponse> JsonGatewayStatusProtocol::makeDeviceStatusResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json j = json::parse(message.getContent());

        const std::string statusStr = j.at(STATUS_RESPONSE_STATE_FIELD).get<std::string>();

        const DeviceStatus status = [&] {
            if (statusStr == STATUS_RESPONSE_STATUS_CONNECTED)
            {
                return DeviceStatus::CONNECTED;
            }
            else if (statusStr == STATUS_RESPONSE_STATUS_SLEEP)
            {
                return DeviceStatus::SLEEP;
            }
            else if (statusStr == STATUS_RESPONSE_STATUS_SERVICE)
            {
                return DeviceStatus::SERVICE;
            }
            else if (statusStr == STATUS_RESPONSE_STATUS_OFFLINE)
            {
                return DeviceStatus::OFFLINE;
            }

            throw std::logic_error("");
        }();

        return std::unique_ptr<DeviceStatusResponse>(new DeviceStatusResponse(status));
    }
    catch (...)
    {
        LOG(TRACE) << "Status protocol: Unable to parse DeviceRegistrationResponseDto: " << message.getContent();
        return nullptr;
    }
}

bool JsonGatewayStatusProtocol::isMessageToPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return isLastWillMessage(message) || StringUtils::startsWith(message.getChannel(), DEVICE_TO_PLATFORM_DIRECTION);
}

bool JsonGatewayStatusProtocol::isMessageFromPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), PLATFORM_TO_DEVICE_DIRECTION);
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

bool JsonGatewayStatusProtocol::isStatusRequestMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), PLATFORM_STATUS_REQUEST_TOPIC_ROOT);
}

bool JsonGatewayStatusProtocol::isStatusConfirmMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), PLATFORM_STATUS_CONFIRM_TOPIC_ROOT);
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

bool JsonGatewayStatusProtocol::isPongMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), PONG_TOPIC_ROOT);
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

    if (StringUtils::startsWith(top, PONG_TOPIC_ROOT))
    {
        auto delimiterPosition = top.find(CHANNEL_DELIMITER);
        if (delimiterPosition == std::string::npos)
        {
            return "";
        }

        return top.substr(delimiterPosition + CHANNEL_DELIMITER.size(), std::string::npos);
    }

    const auto deviceKeyStartPosition = top.find(DEVICE_PATH_PREFIX);
    if (deviceKeyStartPosition != std::string::npos)
    {
        const auto keyEndPosition = top.find(CHANNEL_DELIMITER, deviceKeyStartPosition + DEVICE_PATH_PREFIX.size());

        return top.substr(deviceKeyStartPosition + DEVICE_PATH_PREFIX.size(), keyEndPosition);
    }

    const auto gatewayKeyStartPosition = top.find(GATEWAY_PATH_PREFIX);
    if (gatewayKeyStartPosition == std::string::npos)
    {
        return "";
    }

    const auto keyEndPosition = top.find(CHANNEL_DELIMITER, gatewayKeyStartPosition + GATEWAY_PATH_PREFIX.size());

    return top.substr(gatewayKeyStartPosition + GATEWAY_PATH_PREFIX.size(), keyEndPosition);
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
    catch (...)
    {
        LOG(TRACE) << "Status protocol: Unable to parse content: " << content;
        return {};
    }
}
}    // namespace wolkabout
