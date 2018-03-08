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

#include "connectivity/json/StatusProtocol.h"
#include "model/DeviceStatusResponse.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"
#include <algorithm>

using nlohmann::json;

namespace wolkabout
{
const std::string StatusProtocol::NAME = "StatusProtocol";

const std::string StatusProtocol::CHANNEL_DELIMITER = "/";
const std::string StatusProtocol::CHANNEL_WILDCARD = "#";
const std::string StatusProtocol::GATEWAY_PATH_PREFIX = "g/";
const std::string StatusProtocol::DEVICE_PATH_PREFIX = "d/";
const std::string StatusProtocol::DEVICE_TO_PLATFORM_DIRECTION = "d2p/";
const std::string StatusProtocol::PLATFORM_TO_DEVICE_DIRECTION = "p2d/";

const std::string StatusProtocol::LAST_WILL_TOPIC_ROOT = "lastwill/";
const std::string StatusProtocol::DEVICE_STATUS_REQUEST_TOPIC_ROOT = "p2d/status/";
const std::string StatusProtocol::DEVICE_STATUS_RESPONSE_TOPIC_ROOT = "d2p/status/";

const std::vector<std::string> StatusProtocol::DEVICE_TOPICS = {DEVICE_STATUS_RESPONSE_TOPIC_ROOT + CHANNEL_WILDCARD,
                                                                LAST_WILL_TOPIC_ROOT + CHANNEL_WILDCARD};

const std::vector<std::string> StatusProtocol::PLATFORM_TOPICS = {DEVICE_STATUS_REQUEST_TOPIC_ROOT + CHANNEL_WILDCARD};

const std::string StatusProtocol::STATUS_RESPONSE_STATE_FIELD = "state";
const std::string StatusProtocol::STATUS_RESPONSE_STATUS_CONNECTED = "CONNECTED";
const std::string StatusProtocol::STATUS_RESPONSE_STATUS_SLEEP = "SLEEP";
const std::string StatusProtocol::STATUS_RESPONSE_STATUS_SERVICE = "SERVICE";
const std::string StatusProtocol::STATUS_RESPONSE_STATUS_OFFLINE = "OFFLINE";

void to_json(json& j, const DeviceStatusResponse& p)
{
    const std::string status = [&]() -> std::string {
        switch (p.getStatus())
        {
        case DeviceStatusResponse::Status::CONNECTED:
        {
            return StatusProtocol::STATUS_RESPONSE_STATUS_CONNECTED;
        }
        case DeviceStatusResponse::Status::SLEEP:
        {
            return StatusProtocol::STATUS_RESPONSE_STATUS_SLEEP;
        }
        case DeviceStatusResponse::Status::SERVICE:
        {
            return StatusProtocol::STATUS_RESPONSE_STATUS_SERVICE;
        }
        case DeviceStatusResponse::Status::OFFLINE:
        default:
        {
            return StatusProtocol::STATUS_RESPONSE_STATUS_OFFLINE;
        }
        }
    }();

    j = json{{StatusProtocol::STATUS_RESPONSE_STATE_FIELD, status}};
}

void to_json(json& j, const std::shared_ptr<DeviceStatusResponse>& p)
{
    if (!p)
    {
        return;
    }

    to_json(j, *p);
}

const std::string& StatusProtocol::getName()
{
    return NAME;
}

const std::vector<std::string>& StatusProtocol::getDeviceTopics()
{
    LOG(TRACE) << METHOD_INFO;
    return DEVICE_TOPICS;
}

const std::vector<std::string>& StatusProtocol::getPlatformTopics()
{
    LOG(TRACE) << METHOD_INFO;
    return PLATFORM_TOPICS;
}

std::shared_ptr<Message> StatusProtocol::messageFromDeviceStatusResponse(const std::string& gatewayKey,
                                                                         const std::string& deviceKey,
                                                                         std::shared_ptr<DeviceStatusResponse> response)
{
    LOG(TRACE) << METHOD_INFO;

    const json jPayload(response);
    const std::string topic = DEVICE_STATUS_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                              DEVICE_PATH_PREFIX + deviceKey;

    const std::string payload = jPayload.dump();

    return std::make_shared<Message>(payload, topic);
}

std::shared_ptr<Message> StatusProtocol::messageFromDeviceStatusResponse(const std::string& gatewayKey,
                                                                         const std::string& deviceKey,
                                                                         const DeviceStatusResponse::Status& response)
{
    LOG(TRACE) << METHOD_INFO;

    const json jPayload(DeviceStatusResponse{response});
    const std::string topic = DEVICE_STATUS_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                              DEVICE_PATH_PREFIX + deviceKey;

    const std::string payload = jPayload.dump();

    return std::make_shared<Message>(payload, topic);
}

std::shared_ptr<Message> StatusProtocol::messageFromDeviceStatusRequest(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = DEVICE_STATUS_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey;

    const std::string payload = "";

    return std::make_shared<Message>(payload, topic);
}

std::shared_ptr<DeviceStatusResponse> StatusProtocol::makeDeviceStatusResponse(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json j = json::parse(message->getContent());

        const std::string statusStr = j.at(STATUS_RESPONSE_STATE_FIELD).get<std::string>();

        const DeviceStatusResponse::Status status = [&] {
            if (statusStr == STATUS_RESPONSE_STATUS_CONNECTED)
            {
                return DeviceStatusResponse::Status::CONNECTED;
            }
            else if (statusStr == STATUS_RESPONSE_STATUS_SLEEP)
            {
                return DeviceStatusResponse::Status::SLEEP;
            }
            else if (statusStr == STATUS_RESPONSE_STATUS_SERVICE)
            {
                return DeviceStatusResponse::Status::SERVICE;
            }
            else if (statusStr == STATUS_RESPONSE_STATUS_OFFLINE)
            {
                return DeviceStatusResponse::Status::OFFLINE;
            }

            throw std::logic_error("");
        }();

        return std::make_shared<DeviceStatusResponse>(status);
    }
    catch (...)
    {
        LOG(TRACE) << "Status protocol: Unable to parse DeviceRegistrationResponseDto: " << message->getContent();
        return nullptr;
    }
}

bool StatusProtocol::isMessageToPlatform(const std::string& channel)
{
    LOG(TRACE) << METHOD_INFO;

    return isLastWillMessage(channel) || StringUtils::startsWith(channel, DEVICE_TO_PLATFORM_DIRECTION);
}

bool StatusProtocol::isMessageFromPlatform(const std::string& channel)
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(channel, PLATFORM_TO_DEVICE_DIRECTION);
}

bool StatusProtocol::isStatusResponseMessage(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(topic, DEVICE_STATUS_RESPONSE_TOPIC_ROOT);
}

bool StatusProtocol::isStatusRequestMessage(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(topic, DEVICE_STATUS_REQUEST_TOPIC_ROOT);
}

bool StatusProtocol::isLastWillMessage(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(topic, LAST_WILL_TOPIC_ROOT);
}

std::string StatusProtocol::routeDeviceMessage(const std::string& topic, const std::string& gatewayKey)
{
    LOG(TRACE) << METHOD_INFO;

    auto firstPos = topic.find(CHANNEL_DELIMITER);
    if (firstPos == std::string::npos)
    {
        LOG(TRACE) << "Status protocol: Channel delimiter missing in path: " << topic;
        return "";
    }

    auto secondPos = topic.find(CHANNEL_DELIMITER, firstPos + CHANNEL_DELIMITER.length());
    if (secondPos == std::string::npos)
    {
        LOG(TRACE) << "Status protocol: Channel delimiter missing in path: " << topic;
        return "";
    }

    std::string newTopic = topic;
    return newTopic.insert(secondPos + CHANNEL_DELIMITER.length(), GATEWAY_PATH_PREFIX + gatewayKey);
}

std::string StatusProtocol::routePlatformMessage(const std::string& topic, const std::string& gatewayKey)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string gwTopicPart = GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER;
    if (topic.find(gwTopicPart) != std::string::npos)
    {
        return StringUtils::removeSubstring(topic, gwTopicPart);
    }

    return "";
}

std::string StatusProtocol::extractDeviceKeyFromChannel(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    if (isLastWillMessage(topic))
    {
        return StringUtils::removePrefix(topic, LAST_WILL_TOPIC_ROOT);
    }

    const auto deviceKeyStartPosition = topic.find(DEVICE_PATH_PREFIX);
    if (deviceKeyStartPosition != std::string::npos)
    {
        const auto keyEndPosition = topic.find(CHANNEL_DELIMITER, deviceKeyStartPosition + DEVICE_PATH_PREFIX.size());

        return topic.substr(deviceKeyStartPosition + DEVICE_PATH_PREFIX.size(), keyEndPosition);
    }

    const auto gatewayKeyStartPosition = topic.find(GATEWAY_PATH_PREFIX);
    if (gatewayKeyStartPosition == std::string::npos)
    {
        return "";
    }

    const auto keyEndPosition = topic.find(CHANNEL_DELIMITER, gatewayKeyStartPosition + GATEWAY_PATH_PREFIX.size());

    return topic.substr(gatewayKeyStartPosition + GATEWAY_PATH_PREFIX.size(), keyEndPosition);
}

std::vector<std::string> StatusProtocol::deviceKeysFromContent(const std::string& content)
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
