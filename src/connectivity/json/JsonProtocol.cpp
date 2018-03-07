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
const std::string JsonProtocol::CONFIGURATION_SET_RESPONSE_TOPIC_ROOT = "d2p/configuration_set/";
const std::string JsonProtocol::CONFIGURATION_GET_RESPONSE_TOPIC_ROOT = "d2p/configuration_get/";

const std::string JsonProtocol::ACTUATION_SET_TOPIC_ROOT = "p2d/actuator_set/";
const std::string JsonProtocol::ACTUATION_GET_TOPIC_ROOT = "p2d/actuator_get/";
const std::string JsonProtocol::CONFIGURATION_SET_REQUEST_TOPIC_ROOT = "p2d/configuration_set/";
const std::string JsonProtocol::CONFIGURATION_GET_REQUEST_TOPIC_ROOT = "p2d/configuration_get/";

const std::vector<std::string> JsonProtocol::DEVICE_TOPICS = {SENSOR_READING_TOPIC_ROOT + CHANNEL_WILDCARD,
                                                              EVENTS_TOPIC_ROOT + CHANNEL_WILDCARD,
                                                              ACTUATION_STATUS_TOPIC_ROOT + CHANNEL_WILDCARD};

const std::vector<std::string> JsonProtocol::PLATFORM_TOPICS = {ACTUATION_GET_TOPIC_ROOT + CHANNEL_WILDCARD,
                                                                ACTUATION_SET_TOPIC_ROOT + CHANNEL_WILDCARD};

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

const std::vector<std::string>& JsonProtocol::getDeviceTopics()
{
    return DEVICE_TOPICS;
}

const std::vector<std::string>& JsonProtocol::getPlatformTopics()
{
    return PLATFORM_TOPICS;
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

bool JsonProtocol::isGatewayToPlatformMessage(const std::string& topic)
{
    auto tokens = StringUtils::tokenize(topic, CHANNEL_DELIMITER);

    if (tokens.size() != 6)
    {
        LOG(DEBUG) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != DEVICE_TO_PLATFORM_TYPE)
    {
        LOG(DEBUG) << "Device message dirrection not valid: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != GATEWAY_TYPE)
    {
        LOG(DEBUG) << "Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_REFERENCE_TYPE_POS] != REFERENCE_TYPE)
    {
        LOG(DEBUG) << "Reference perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool JsonProtocol::isPlatformToGatewayMessage(const std::string& topic)
{
    auto tokens = StringUtils::tokenize(topic, CHANNEL_DELIMITER);

    if (tokens.size() != 6)
    {
        LOG(DEBUG) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != PLATFORM_TO_DEVICE_TYPE)
    {
        LOG(DEBUG) << "Device message dirrection not valid: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != GATEWAY_TYPE)
    {
        LOG(DEBUG) << "Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_REFERENCE_TYPE_POS] != REFERENCE_TYPE)
    {
        LOG(DEBUG) << "Device perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool JsonProtocol::isDeviceToPlatformMessage(const std::string& topic)
{
    auto tokens = StringUtils::tokenize(topic, CHANNEL_DELIMITER);

    if (tokens.size() < 6)
    {
        LOG(DEBUG) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != DEVICE_TO_PLATFORM_TYPE)
    {
        LOG(DEBUG) << "Device message dirrection not valid: " << topic;
        return false;
    }

    if (tokens[DEVICE_TYPE_POS] != DEVICE_TYPE)
    {
        LOG(DEBUG) << "Device perfix missing in path: " << topic;
        return false;
    }

    if (tokens[DEVICE_REFERENCE_TYPE_POS] != REFERENCE_TYPE)
    {
        LOG(DEBUG) << "Reference perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool JsonProtocol::isPlatformToDeviceMessage(const std::string& topic)
{
    auto tokens = StringUtils::tokenize(topic, CHANNEL_DELIMITER);

    if (tokens.size() != 8)
    {
        LOG(DEBUG) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != PLATFORM_TO_DEVICE_TYPE)
    {
        LOG(DEBUG) << "Device message dirrection not valid: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != GATEWAY_TYPE)
    {
        LOG(DEBUG) << "Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_DEVICE_TYPE_POS] != DEVICE_TYPE)
    {
        LOG(DEBUG) << "Device perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_DEVICE_REFERENCE_TYPE_POS] != REFERENCE_TYPE)
    {
        LOG(DEBUG) << "Reference perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool JsonProtocol::isActuatorSetMessage(const std::string& topic)
{
    return StringUtils::startsWith(topic, ACTUATION_SET_TOPIC_ROOT);
}

bool JsonProtocol::isActuatorGetMessage(const std::string& topic)
{
    return StringUtils::startsWith(topic, ACTUATION_GET_TOPIC_ROOT);
}

std::string JsonProtocol::routePlatformMessage(const std::string& topic, const std::string& gatewayKey)
{
    const std::string gwTopicPart = GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER;
    if (topic.find(gwTopicPart) != std::string::npos)
    {
        return StringUtils::removeSubstring(topic, gwTopicPart);
    }

    return "";
}

std::string JsonProtocol::routeDeviceMessage(const std::string& topic, const std::string& gatewayKey)
{
    auto firstPos = topic.find(CHANNEL_DELIMITER);
    if (firstPos == std::string::npos)
    {
        return "";
    }

    auto secondPos = topic.find(CHANNEL_DELIMITER, firstPos + CHANNEL_DELIMITER.length());
    if (secondPos == std::string::npos)
    {
        return "";
    }

    std::string newTopic = topic;
    return newTopic.insert(secondPos + CHANNEL_DELIMITER.length(),
                           GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER);
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

        return topic.substr(deviceKeyStartPosition + devicePathPrefix.size(), keyEndPosition);
    }

    const std::string gatewayPathPrefix = CHANNEL_DELIMITER + GATEWAY_PATH_PREFIX;

    const auto gatewayKeyStartPosition = topic.find(gatewayPathPrefix);
    if (gatewayKeyStartPosition == std::string::npos)
    {
        return "";
    }

    const auto keyEndPosition = topic.find(CHANNEL_DELIMITER, gatewayKeyStartPosition + gatewayPathPrefix.size());

    return topic.substr(gatewayKeyStartPosition + gatewayPathPrefix.size(), keyEndPosition);
}
}    // namespace wolkabout
