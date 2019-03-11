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
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"

#include <algorithm>

using nlohmann::json;

namespace wolkabout
{
const std::string JsonGatewayDataProtocol::NAME = "JsonProtocol";

const std::string JsonGatewayDataProtocol::CHANNEL_DELIMITER = "/";
const std::string JsonGatewayDataProtocol::CHANNEL_MULTI_LEVEL_WILDCARD = "#";
const std::string JsonGatewayDataProtocol::CHANNEL_SINGLE_LEVEL_WILDCARD = "+";

const std::string JsonGatewayDataProtocol::GATEWAY_TYPE = "g";
const std::string JsonGatewayDataProtocol::DEVICE_TYPE = "d";
const std::string JsonGatewayDataProtocol::REFERENCE_TYPE = "r";
const std::string JsonGatewayDataProtocol::DEVICE_TO_PLATFORM_TYPE = "d2p";
const std::string JsonGatewayDataProtocol::PLATFORM_TO_DEVICE_TYPE = "p2d";

const std::string JsonGatewayDataProtocol::GATEWAY_PATH_PREFIX = "g/";
const std::string JsonGatewayDataProtocol::DEVICE_PATH_PREFIX = "d/";
const std::string JsonGatewayDataProtocol::REFERENCE_PATH_PREFIX = "r/";
const std::string JsonGatewayDataProtocol::DEVICE_TO_PLATFORM_DIRECTION = "d2p/";
const std::string JsonGatewayDataProtocol::PLATFORM_TO_DEVICE_DIRECTION = "p2d/";

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

const std::string& JsonGatewayDataProtocol::getName() const
{
    return NAME;
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

std::unique_ptr<Message> JsonGatewayDataProtocol::makeMessage(const std::string& gatewayKey,
                                                              const ActuatorStatus& actuatorStatus) const
{
    const json jPayload(actuatorStatus);
    const std::string topic = ACTUATION_STATUS_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                              REFERENCE_PATH_PREFIX + actuatorStatus.getReference();
    const std::string payload = jPayload.dump();

    return std::unique_ptr<Message>(new Message(payload, topic));
}

std::unique_ptr<Message> JsonGatewayDataProtocol::makeMessage(const std::string& deviceKey,
                                                              const ActuatorGetCommand& command) const
{
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

std::unique_ptr<ActuatorSetCommand> JsonGatewayDataProtocol::makeActuatorSetCommand(const Message& message) const
{
    try
    {
        json j = json::parse(message.getContent());

        const std::string value = [&]() -> std::string {
            if (j.find("value") != j.end())
            {
                return j.at("value").get<std::string>();
            }

            return "";
        }();

        const auto reference = extractReferenceFromChannel(message.getChannel());

        return std::unique_ptr<ActuatorSetCommand>(new ActuatorSetCommand(reference, value));
    }
    catch (...)
    {
        LOG(DEBUG) << "Unable to parse ActuatorSetCommand: " << message.getContent();
        return nullptr;
    }
}

std::unique_ptr<ActuatorGetCommand> JsonGatewayDataProtocol::makeActuatorGetCommand(const Message& message) const
{
    try
    {
        const auto reference = extractReferenceFromChannel(message.getChannel());

        if (reference.empty())
        {
            return nullptr;
        }

        return std::unique_ptr<ActuatorGetCommand>(new ActuatorGetCommand(reference));
    }
    catch (...)
    {
        LOG(DEBUG) << "Unable to parse ActuatorGetCommand: " << message.getContent();
        return nullptr;
    }
}

bool JsonGatewayDataProtocol::isMessageToPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_TO_PLATFORM_DIRECTION);
}

bool JsonGatewayDataProtocol::isMessageFromPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), PLATFORM_TO_DEVICE_DIRECTION);
}

bool JsonGatewayDataProtocol::isActuatorSetMessage(const Message& message) const
{
    return StringUtils::startsWith(message.getChannel(), ACTUATION_SET_TOPIC_ROOT);
}

bool JsonGatewayDataProtocol::isActuatorGetMessage(const Message& message) const
{
    return StringUtils::startsWith(message.getChannel(), ACTUATION_GET_TOPIC_ROOT);
}

bool JsonGatewayDataProtocol::isConfigurationSetMessage(const Message& message) const
{
    return StringUtils::startsWith(message.getChannel(), CONFIGURATION_SET_REQUEST_TOPIC_ROOT);
}

bool JsonGatewayDataProtocol::isConfigurationGetMessage(const Message& message) const
{
    return StringUtils::startsWith(message.getChannel(), CONFIGURATION_GET_REQUEST_TOPIC_ROOT);
}

bool JsonGatewayDataProtocol::isSensorReadingMessage(const Message& message) const
{
    return StringUtils::startsWith(message.getChannel(), SENSOR_READING_TOPIC_ROOT);
}

bool JsonGatewayDataProtocol::isAlarmMessage(const Message& message) const
{
    return StringUtils::startsWith(message.getChannel(), EVENTS_TOPIC_ROOT);
}

bool JsonGatewayDataProtocol::isActuatorStatusMessage(const Message& message) const
{
    return StringUtils::startsWith(message.getChannel(), ACTUATION_STATUS_TOPIC_ROOT);
}

bool JsonGatewayDataProtocol::isConfigurationCurrentMessage(const Message& message) const
{
    return StringUtils::startsWith(message.getChannel(), CONFIGURATION_RESPONSE_TOPIC_ROOT);
}

std::string JsonGatewayDataProtocol::routePlatformToDeviceMessage(const std::string& topic,
                                                                  const std::string& gatewayKey) const
{
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

std::string JsonGatewayDataProtocol::routePlatformToGatewayMessage(const std::string& topic) const
{
    const std::string deviceTopicPart = CHANNEL_DELIMITER + DEVICE_PATH_PREFIX;
    const std::string gatewayTopicPart = CHANNEL_DELIMITER + GATEWAY_PATH_PREFIX;

    const auto position = topic.find(gatewayTopicPart);
    if (position != std::string::npos)
    {
        std::string routedTopic = topic;
        return routedTopic.replace(position, gatewayTopicPart.size(), deviceTopicPart);
    }

    return "";
}

std::string JsonGatewayDataProtocol::routeGatewayToPlatformMessage(const std::string& topic) const
{
    const std::string deviceTopicPart = CHANNEL_DELIMITER + DEVICE_PATH_PREFIX;
    const std::string gatewayTopicPart = CHANNEL_DELIMITER + GATEWAY_PATH_PREFIX;

    const auto position = topic.find(deviceTopicPart);
    if (position != std::string::npos)
    {
        std::string routedTopic = topic;
        return routedTopic.replace(position, deviceTopicPart.size(), gatewayTopicPart);
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
