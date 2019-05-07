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

#include "protocol/json/JsonGatewayDataProtocol.h"
#include "model/ActuatorGetCommand.h"
#include "model/ActuatorSetCommand.h"
#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/Message.h"
#include "protocol/json/Json.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"

#include <algorithm>

using nlohmann::json;

namespace wolkabout
{
const std::string JsonGatewayDataProtocol::SENSOR_READING_TOPIC_ROOT = "d2p/sensor_reading/";
const std::string JsonGatewayDataProtocol::EVENTS_TOPIC_ROOT = "d2p/events/";
const std::string JsonGatewayDataProtocol::ACTUATION_STATUS_TOPIC_ROOT = "d2p/actuator_status/";
const std::string JsonGatewayDataProtocol::CONFIGURATION_RESPONSE_TOPIC_ROOT = "d2p/configuration_get/";

const std::string JsonGatewayDataProtocol::ACTUATION_SET_TOPIC_ROOT = "p2d/actuator_set/";
const std::string JsonGatewayDataProtocol::ACTUATION_GET_TOPIC_ROOT = "p2d/actuator_get/";
const std::string JsonGatewayDataProtocol::CONFIGURATION_SET_REQUEST_TOPIC_ROOT = "p2d/configuration_set/";
const std::string JsonGatewayDataProtocol::CONFIGURATION_GET_REQUEST_TOPIC_ROOT = "p2d/configuration_get/";

static void to_json(json& j, const ActuatorStatus& p)
{
    const std::string status = [&]() -> std::string {
        if (p.getState() == ActuatorStatus::State::READY)
        {
            return "READY";
        }
        else if (p.getState() == ActuatorStatus::State::BUSY)
        {
            return "BUSY";
        }
        else if (p.getState() == ActuatorStatus::State::ERROR)
        {
            return "ERROR";
        }

        return "ERROR";
    }();

    j = json{{"status", status}, {"value", p.getValue()}};
}

static void to_json(json& j, const std::shared_ptr<ActuatorStatus>& p)
{
    if (!p)
    {
        return;
    }

    to_json(j, *p);
}

std::vector<std::string> JsonGatewayDataProtocol::getInboundChannels() const
{
    return {SENSOR_READING_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_SINGLE_LEVEL_WILDCARD + CHANNEL_DELIMITER +
              REFERENCE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            EVENTS_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_SINGLE_LEVEL_WILDCARD + CHANNEL_DELIMITER +
              REFERENCE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            ACTUATION_STATUS_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_SINGLE_LEVEL_WILDCARD + CHANNEL_DELIMITER +
              REFERENCE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            CONFIGURATION_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewayDataProtocol::getInboundChannelsForDevice(const std::string& deviceKey) const
{
    return {SENSOR_READING_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey + CHANNEL_DELIMITER + REFERENCE_PATH_PREFIX +
              CHANNEL_MULTI_LEVEL_WILDCARD,
            EVENTS_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey + CHANNEL_DELIMITER + REFERENCE_PATH_PREFIX +
              CHANNEL_MULTI_LEVEL_WILDCARD,
            ACTUATION_STATUS_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey + CHANNEL_DELIMITER + REFERENCE_PATH_PREFIX +
              CHANNEL_MULTI_LEVEL_WILDCARD,
            CONFIGURATION_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey};
}

std::unique_ptr<Message> JsonGatewayDataProtocol::makeMessage(const std::string& deviceKey,
                                                              const ActuatorGetCommand& command) const
{
    LOG(TRACE) << METHOD_INFO;

    std::string topic;
    if (!deviceKey.empty())
    {
        topic = ACTUATION_GET_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey + CHANNEL_DELIMITER + REFERENCE_PATH_PREFIX +
                command.getReference();
    }
    else
    {
        topic = ACTUATION_GET_TOPIC_ROOT + DEVICE_PATH_PREFIX;
    }

    return std::unique_ptr<Message>(new Message("", topic));
}

bool JsonGatewayDataProtocol::isSensorReadingMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), SENSOR_READING_TOPIC_ROOT);
}

bool JsonGatewayDataProtocol::isAlarmMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), EVENTS_TOPIC_ROOT);
}

bool JsonGatewayDataProtocol::isActuatorStatusMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), ACTUATION_STATUS_TOPIC_ROOT);
}

bool JsonGatewayDataProtocol::isConfigurationCurrentMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), CONFIGURATION_RESPONSE_TOPIC_ROOT);
}

std::string JsonGatewayDataProtocol::routePlatformToDeviceMessage(const std::string& topic,
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

std::string JsonGatewayDataProtocol::routeDeviceToPlatformMessage(const std::string& topic,
                                                                  const std::string& gatewayKey) const
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

std::string JsonGatewayDataProtocol::extractReferenceFromChannel(const std::string& topic) const
{
    LOG(TRACE) << METHOD_INFO;

    std::string top{topic};

    if (StringUtils::endsWith(topic, CHANNEL_DELIMITER))
    {
        top = top.substr(0, top.size() - CHANNEL_DELIMITER.size());
    }

    const std::string referencePathPrefix = CHANNEL_DELIMITER + REFERENCE_PATH_PREFIX;

    auto pos = top.rfind(referencePathPrefix);

    if (pos != std::string::npos)
    {
        return top.substr(pos + referencePathPrefix.size(), std::string::npos);
    }

    return "";
}

std::string JsonGatewayDataProtocol::extractDeviceKeyFromChannel(const std::string& topic) const
{
    LOG(TRACE) << METHOD_INFO;

    const std::string devicePathPrefix = CHANNEL_DELIMITER + DEVICE_PATH_PREFIX;

    const auto deviceKeyStartPosition = topic.find(devicePathPrefix);
    if (deviceKeyStartPosition != std::string::npos)
    {
        const auto keyEndPosition = topic.find(CHANNEL_DELIMITER, deviceKeyStartPosition + devicePathPrefix.size());

        const auto pos = deviceKeyStartPosition + devicePathPrefix.size();

        return topic.substr(pos, keyEndPosition - pos);
    }

    const std::string gatewayPathPrefix = CHANNEL_DELIMITER + GATEWAY_PATH_PREFIX;

    const auto gatewayKeyStartPosition = topic.find(gatewayPathPrefix);
    if (gatewayKeyStartPosition == std::string::npos)
    {
        return "";
    }

    const auto keyEndPosition = topic.find(CHANNEL_DELIMITER, gatewayKeyStartPosition + gatewayPathPrefix.size());

    const auto pos = gatewayKeyStartPosition + gatewayPathPrefix.size();

    return topic.substr(pos, keyEndPosition - pos);
}
}    // namespace wolkabout
