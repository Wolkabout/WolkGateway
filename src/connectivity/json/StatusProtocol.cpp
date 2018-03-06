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
#include "connectivity/Channels.h"
#include "model/DeviceStatusResponse.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"
#include <algorithm>

using nlohmann::json;

namespace wolkabout
{
const std::string StatusProtocol::m_name = "StatusProtocol";

const std::vector<std::string> StatusProtocol::m_deviceTopics = {
  Channel::DEVICE_STATUS_RESPONSE_TOPIC_ROOT + Channel::CHANNEL_WILDCARD, Channel::LAST_WILL_TOPIC_ROOT};

const std::vector<std::string> StatusProtocol::m_platformTopics = {Channel::DEVICE_STATUS_REQUEST_TOPIC_ROOT +
                                                                   Channel::CHANNEL_WILDCARD};

const std::vector<std::string> StatusProtocol::m_deviceMessageTypes = {Channel::DEVICE_STATUS_TYPE,
                                                                       Channel::LAST_WILL_TYPE};

const std::vector<std::string> StatusProtocol::m_platformMessageTypes = {Channel::DEVICE_STATUS_TYPE};

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

const std::vector<std::string>& StatusProtocol::getDeviceTopics()
{
    LOG(TRACE) << METHOD_INFO;
    return m_deviceTopics;
}

const std::vector<std::string>& StatusProtocol::getPlatformTopics()
{
    LOG(TRACE) << METHOD_INFO;
    return m_platformTopics;
}

std::shared_ptr<Message> StatusProtocol::messageFromDeviceStatusResponse(const std::string& gatewayKey,
                                                                         const std::string& deviceKey,
                                                                         std::shared_ptr<DeviceStatusResponse> response)
{
    LOG(TRACE) << METHOD_INFO;

    const json jPayload(response);
    const std::string topic = Channel::DEVICE_STATUS_RESPONSE_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                              Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER +
                              Channel::DEVICE_PATH_PREFIX + deviceKey + Channel::CHANNEL_DELIMITER;

    const std::string payload = jPayload.dump();

    return std::make_shared<Message>(payload, topic);
}

std::shared_ptr<Message> StatusProtocol::messageFromDeviceStatusResponse(const std::string& gatewayKey,
                                                                         const std::string& deviceKey,
                                                                         const DeviceStatusResponse::Status& response)
{
    LOG(TRACE) << METHOD_INFO;

    const json jPayload(DeviceStatusResponse{response});
    const std::string topic = Channel::DEVICE_STATUS_RESPONSE_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                              Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER +
                              Channel::DEVICE_PATH_PREFIX + deviceKey + Channel::CHANNEL_DELIMITER;

    const std::string payload = jPayload.dump();

    return std::make_shared<Message>(payload, topic);
}

std::shared_ptr<Message> StatusProtocol::messageFromDeviceStatusRequest(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic =
      Channel::DEVICE_STATUS_REQUEST_TOPIC_ROOT + Channel::DEVICE_PATH_PREFIX + deviceKey + Channel::CHANNEL_DELIMITER;

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

bool StatusProtocol::isGatewayToPlatformMessage(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() < GATEWAY_KEY_POS + 1)
    {
        LOG(TRACE) << "Status protocol: Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::DEVICE_TO_PLATFORM_DIRECTION)
    {
        LOG(TRACE) << "Status protocol: Device message dirrection not valid: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(TRACE) << "Status protocol: Gateway perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool StatusProtocol::isDeviceToPlatformMessage(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() < DEVICE_KEY_POS + 1)
    {
        LOG(TRACE) << "Status protocol: Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::DEVICE_TO_PLATFORM_DIRECTION)
    {
        LOG(TRACE) << "Status protocol: Device message dirrection not valid: " << topic;
        return false;
    }

    if (tokens[DEVICE_TYPE_POS] != Channel::DEVICE_PATH_PREFIX)
    {
        LOG(TRACE) << "Status protocol: Device perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool StatusProtocol::isPlatformToDeviceMessage(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() <= GATEWAY_DEVICE_KEY_POS + 1)
    {
        LOG(TRACE) << "Status protocol: Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::PLATFORM_TO_DEVICE_DIRECTION)
    {
        LOG(TRACE) << "Status protocol: Dirrection mismatch in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(TRACE) << "Status protocol: Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_DEVICE_TYPE_POS] != Channel::DEVICE_PATH_PREFIX)
    {
        LOG(TRACE) << "Status protocol: Device perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool StatusProtocol::isStatusResponseMessage(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(topic, Channel::DEVICE_STATUS_RESPONSE_TOPIC_ROOT);
}

bool StatusProtocol::isStatusRequestMessage(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(topic, Channel::DEVICE_STATUS_REQUEST_TOPIC_ROOT);
}

bool StatusProtocol::isLastWillMessage(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(topic, Channel::LAST_WILL_TOPIC_ROOT);
}

std::string StatusProtocol::routeDeviceMessage(const std::string& topic, const std::string& gatewayKey)
{
    LOG(TRACE) << METHOD_INFO;

    auto firstPos = topic.find(Channel::CHANNEL_DELIMITER);
    if (firstPos == std::string::npos)
    {
        LOG(TRACE) << "Status protocol: Channel delimiter missing in path: " << topic;
        return "";
    }

    auto secondPos = topic.find(Channel::CHANNEL_DELIMITER, firstPos + Channel::CHANNEL_DELIMITER.length());
    if (secondPos == std::string::npos)
    {
        LOG(TRACE) << "Status protocol: Channel delimiter missing in path: " << topic;
        return "";
    }

    std::string newTopic = topic;
    return newTopic.insert(
      secondPos + Channel::CHANNEL_DELIMITER.length(),
      Channel::GATEWAY_PATH_PREFIX + Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER);
}

std::string StatusProtocol::routePlatformMessage(const std::string& topic, const std::string& gatewayKey)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string gwTopicPart =
      Channel::GATEWAY_PATH_PREFIX + Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER;
    if (topic.find(gwTopicPart) != std::string::npos)
    {
        return StringUtils::removeSubstring(topic, gwTopicPart);
    }

    return "";
}

std::string StatusProtocol::deviceKeyFromTopic(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    if (isLastWillMessage(topic))
    {
        return StringUtils::removePrefix(topic, Channel::LAST_WILL_TOPIC_ROOT);
    }

    const std::string devicePathPrefix = Channel::DEVICE_PATH_PREFIX + Channel::CHANNEL_DELIMITER;

    const auto keyStartPosition = topic.find(devicePathPrefix);
    if (keyStartPosition == std::string::npos)
    {
        LOG(TRACE) << "Status Protocol: Channel does not cointain device path prefix: " << topic;
        return "";
    }

    const auto keyEndPosition = topic.find(Channel::CHANNEL_DELIMITER, keyStartPosition + devicePathPrefix.size());
    if (keyEndPosition == std::string::npos)
    {
        return topic.substr(keyStartPosition + devicePathPrefix.size(), std::string::npos);
    }

    return topic.substr(keyStartPosition + devicePathPrefix.size(), keyEndPosition);
}

std::string StatusProtocol::gatewayKeyFromTopic(const std::string& topic)
{
    LOG(TRACE) << METHOD_INFO;

    if (isLastWillMessage(topic))
    {
        return StringUtils::removePrefix(topic, Channel::LAST_WILL_TOPIC_ROOT);
    }

    const std::string gatewayPathPrefix = Channel::GATEWAY_PATH_PREFIX + Channel::CHANNEL_DELIMITER;

    const auto keyStartPosition = topic.find(gatewayPathPrefix);
    if (keyStartPosition == std::string::npos)
    {
        LOG(TRACE) << "Status Protocol: Channel does not cointain gateway path prefix: " << topic;
        return "";
    }

    const auto keyEndPosition = topic.find(Channel::CHANNEL_DELIMITER, keyStartPosition + gatewayPathPrefix.size());
    if (keyEndPosition == std::string::npos)
    {
        return topic.substr(keyStartPosition + gatewayPathPrefix.size(), std::string::npos);
    }

    return topic.substr(keyStartPosition + gatewayPathPrefix.size(), keyEndPosition);
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
