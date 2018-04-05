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

#include "connectivity/json/JsonProtocol.h"
#include "model/ActuatorGetCommand.h"
#include "model/ActuatorSetCommand.h"
#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/Message.h"
#include "model/SensorReading.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"
#include <algorithm>

using nlohmann::json;

namespace wolkabout
{
const std::string JsonProtocol::NAME = "JsonProtocol";

const std::string JsonProtocol::CHANNEL_DELIMITER = "/";
const std::string JsonProtocol::CHANNEL_WILDCARD = "#";

const std::string JsonProtocol::GATEWAY_TYPE = "g";
const std::string JsonProtocol::DEVICE_TYPE = "d";
const std::string JsonProtocol::REFERENCE_TYPE = "r";
const std::string JsonProtocol::DEVICE_TO_PLATFORM_TYPE = "d2p";
const std::string JsonProtocol::PLATFORM_TO_DEVICE_TYPE = "p2d";

const std::string JsonProtocol::GATEWAY_PATH_PREFIX = "g/";
const std::string JsonProtocol::DEVICE_PATH_PREFIX = "d/";
const std::string JsonProtocol::REFERENCE_PATH_PREFIX = "r/";
const std::string JsonProtocol::DEVICE_TO_PLATFORM_DIRECTION = "d2p/";
const std::string JsonProtocol::PLATFORM_TO_DEVICE_DIRECTION = "p2d/";

const std::string JsonProtocol::SENSOR_READING_TOPIC_ROOT = "d2p/sensor_reading/";
const std::string JsonProtocol::EVENTS_TOPIC_ROOT = "d2p/events/";
const std::string JsonProtocol::ACTUATION_STATUS_TOPIC_ROOT = "d2p/actuator_status/";
const std::string JsonProtocol::CONFIGURATION_RESPONSE_TOPIC_ROOT = "d2p/configuration_get/";

const std::string JsonProtocol::ACTUATION_SET_TOPIC_ROOT = "p2d/actuator_set/";
const std::string JsonProtocol::ACTUATION_GET_TOPIC_ROOT = "p2d/actuator_get/";
const std::string JsonProtocol::CONFIGURATION_SET_REQUEST_TOPIC_ROOT = "p2d/configuration_set/";
const std::string JsonProtocol::CONFIGURATION_GET_REQUEST_TOPIC_ROOT = "p2d/configuration_get/";

const std::vector<std::string> JsonProtocol::DEVICE_CHANNELS = {
  SENSOR_READING_TOPIC_ROOT + CHANNEL_WILDCARD, EVENTS_TOPIC_ROOT + CHANNEL_WILDCARD,
  ACTUATION_STATUS_TOPIC_ROOT + CHANNEL_WILDCARD, CONFIGURATION_RESPONSE_TOPIC_ROOT + CHANNEL_WILDCARD};

const std::vector<std::string> JsonProtocol::PLATFORM_CHANNELS = {
  ACTUATION_GET_TOPIC_ROOT + CHANNEL_WILDCARD, ACTUATION_SET_TOPIC_ROOT + CHANNEL_WILDCARD,
  CONFIGURATION_GET_REQUEST_TOPIC_ROOT + CHANNEL_WILDCARD, CONFIGURATION_SET_REQUEST_TOPIC_ROOT + CHANNEL_WILDCARD};

void from_json(const json& j, SensorReading& reading)
{
    const std::string value = [&]() -> std::string {
        if (j.find("value") != j.end())
        {
            return j.at("value").get<std::string>();
        }

        return "";
    }();

    reading = SensorReading("", value);
}

void to_json(json& j, const SensorReading& p)
{
    if (p.getRtc() == 0)
    {
        j = json{{"data", p.getValue()}};
    }
    else
    {
        j = json{{"utc", p.getRtc()}, {"data", p.getValue()}};
    }
}

void to_json(json& j, const std::shared_ptr<SensorReading>& p)
{
    if (!p)
    {
        return;
    }

    to_json(j, *p);
}

void to_json(json& j, const Alarm& p)
{
    if (p.getRtc() == 0)
    {
        j = json{{"data", p.getValue()}};
    }
    else
    {
        j = json{{"utc", p.getRtc()}, {"data", p.getValue()}};
    }
}

void to_json(json& j, const std::shared_ptr<Alarm>& p)
{
    if (!p)
    {
        return;
    }

    to_json(j, *p);
}

void to_json(json& j, const ActuatorStatus& p)
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

void to_json(json& j, const std::shared_ptr<ActuatorStatus>& p)
{
    if (!p)
    {
        return;
    }

    to_json(j, *p);
}

const std::string& JsonProtocol::getName()
{
    return NAME;
}

const std::vector<std::string>& JsonProtocol::getDeviceChannels()
{
    return DEVICE_CHANNELS;
}

const std::vector<std::string>& JsonProtocol::getPlatformChannels()
{
    return PLATFORM_CHANNELS;
}

std::shared_ptr<Message> JsonProtocol::make(const std::string& gatewayKey,
                                            std::shared_ptr<ActuatorStatus> actuatorStatus)
{
    return make(gatewayKey, *actuatorStatus);
}

std::shared_ptr<Message> JsonProtocol::make(const std::string& gatewayKey, const ActuatorStatus& actuatorStatus)
{
    // JSON_SINGLE allows only 1 ActuatorStatus per Message
    const json jPayload(actuatorStatus);
    const std::string topic = ACTUATION_STATUS_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                              REFERENCE_PATH_PREFIX + actuatorStatus.getReference();
    const std::string payload = jPayload.dump();

    return std::make_shared<Message>(payload, topic);
}

bool JsonProtocol::fromMessage(std::shared_ptr<Message> message, ActuatorSetCommand& command)
{
    try
    {
        json j = json::parse(message->getContent());

        const std::string value = [&]() -> std::string {
            if (j.find("value") != j.end())
            {
                return j.at("value").get<std::string>();
            }

            return "";
        }();

        const auto reference = extractReferenceFromChannel(message->getChannel());

        command = ActuatorSetCommand(reference, value);
    }
    catch (...)
    {
        LOG(DEBUG) << "Unable to parse ActuatorSetCommand: " << message->getContent();
        return false;
    }

    return true;
}

bool JsonProtocol::fromMessage(std::shared_ptr<Message> message, ActuatorGetCommand& command)
{
    try
    {
        const auto reference = extractReferenceFromChannel(message->getChannel());

        if (reference.empty())
        {
            return false;
        }

        command = ActuatorGetCommand(reference);
    }
    catch (...)
    {
        LOG(DEBUG) << "Unable to parse ActuatorGetCommand: " << message->getContent();
        return false;
    }

    return true;
}

bool JsonProtocol::isMessageToPlatform(const std::string& channel)
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(channel, DEVICE_TO_PLATFORM_DIRECTION);
}

bool JsonProtocol::isMessageFromPlatform(const std::string& channel)
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(channel, PLATFORM_TO_DEVICE_DIRECTION);
}

bool JsonProtocol::isActuatorSetMessage(const std::string& channel)
{
    return StringUtils::startsWith(channel, ACTUATION_SET_TOPIC_ROOT);
}

bool JsonProtocol::isActuatorGetMessage(const std::string& channel)
{
    return StringUtils::startsWith(channel, ACTUATION_GET_TOPIC_ROOT);
}

bool JsonProtocol::isConfigurationSetMessage(const std::string& channel)
{
    return StringUtils::startsWith(channel, CONFIGURATION_SET_REQUEST_TOPIC_ROOT);
}

bool JsonProtocol::isConfigurationGetMessage(const std::string& channel)
{
    return StringUtils::startsWith(channel, CONFIGURATION_GET_REQUEST_TOPIC_ROOT);
}

bool JsonProtocol::isSensorReadingMessage(const std::string& channel)
{
    return StringUtils::startsWith(channel, SENSOR_READING_TOPIC_ROOT);
}

bool JsonProtocol::isAlarmMessage(const std::string& channel)
{
    return StringUtils::startsWith(channel, EVENTS_TOPIC_ROOT);
}

bool JsonProtocol::isActuatorStatusMessage(const std::string& channel)
{
    return StringUtils::startsWith(channel, ACTUATION_STATUS_TOPIC_ROOT);
}

bool JsonProtocol::isConfigurationCurrentMessage(const std::string& channel)
{
    return StringUtils::startsWith(channel, CONFIGURATION_RESPONSE_TOPIC_ROOT);
}

std::string JsonProtocol::routePlatformToDeviceMessage(const std::string& topic, const std::string& gatewayKey)
{
    const std::string gwTopicPart = GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER;
    if (topic.find(gwTopicPart) != std::string::npos)
    {
        return StringUtils::removeSubstring(topic, gwTopicPart);
    }

    return "";
}

std::string JsonProtocol::routeDeviceToPlatformMessage(const std::string& topic, const std::string& gatewayKey)
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

std::string JsonProtocol::routePlatformToGatewayMessage(const std::string& topic)
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

std::string JsonProtocol::routeGatewayToPlatformMessage(const std::string& topic)
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

std::string JsonProtocol::extractReferenceFromChannel(const std::string& topic)
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

std::string JsonProtocol::extractDeviceKeyFromChannel(const std::string& topic)
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
