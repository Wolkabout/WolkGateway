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

#include "connectivity/json/JsonSingleProtocol.h"
#include "connectivity/Channels.h"
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

JsonSingleProtocol::JsonSingleProtocol()
: m_devicTopics{Channel::SENSOR_READING_TOPIC_ROOT + Channel::CHANNEL_WILDCARD,
                Channel::EVENTS_TOPIC_ROOT + Channel::CHANNEL_WILDCARD,
                Channel::ACTUATION_STATUS_TOPIC_ROOT + Channel::CHANNEL_WILDCARD}
, m_platformTopics{Channel::ACTUATION_GET_TOPIC_ROOT + Channel::CHANNEL_WILDCARD,
                   Channel::ACTUATION_SET_TOPIC_ROOT + Channel::CHANNEL_WILDCARD}
, m_deviceMessageTypes{Channel::SENSOR_READING_TYPE, Channel::EVENT_TYPE, Channel::ACTUATION_STATUS_TYPE}
, m_platformMessageTypes{Channel::ACTUATION_GET_TYPE, Channel::ACTUATION_SET_TYPE}
{
}

std::vector<std::string> JsonSingleProtocol::getDeviceTopics()
{
    return m_devicTopics;
}

std::vector<std::string> JsonSingleProtocol::getPlatformTopics()
{
    return m_platformTopics;
}

std::shared_ptr<Message> JsonSingleProtocol::make(const std::string& gatewayKey,
                                                  std::vector<std::shared_ptr<SensorReading>> sensorReadings)
{
    if (sensorReadings.size() == 0)
    {
        return nullptr;
    }

    const json jPayload(sensorReadings);
    const std::string topic = Channel::SENSOR_READING_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                              Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER +
                              Channel::REFERENCE_PATH_PREFIX + Channel::CHANNEL_DELIMITER +
                              sensorReadings.front()->getReference();
    const std::string payload = jPayload.dump();

    return std::make_shared<Message>(payload, topic);
}

std::shared_ptr<Message> JsonSingleProtocol::make(const std::string& gatewayKey,
                                                  std::vector<std::shared_ptr<Alarm>> alarms)
{
    if (alarms.size() == 0)
    {
        return nullptr;
    }

    const json jPayload(alarms);
    const std::string topic = Channel::EVENTS_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + Channel::CHANNEL_DELIMITER +
                              gatewayKey + Channel::CHANNEL_DELIMITER + Channel::REFERENCE_PATH_PREFIX +
                              Channel::CHANNEL_DELIMITER + alarms.front()->getReference();
    const std::string payload = jPayload.dump();

    return std::make_shared<Message>(payload, topic);
}

std::shared_ptr<Message> JsonSingleProtocol::make(const std::string& gatewayKey,
                                                  std::shared_ptr<ActuatorStatus> actuatorStatuses)
{
    // JSON_SINGLE allows only 1 ActuatorStatus per Message
    const json jPayload(actuatorStatuses);
    const std::string topic = Channel::ACTUATION_STATUS_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                              Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER +
                              Channel::REFERENCE_PATH_PREFIX + Channel::CHANNEL_DELIMITER +
                              actuatorStatuses->getReference();
    const std::string payload = jPayload.dump();

    return std::make_shared<Message>(payload, topic);
}

std::shared_ptr<Message> JsonSingleProtocol::make(const std::string& gatewayKey, const ActuatorStatus& actuatorStatuses)
{
    // JSON_SINGLE allows only 1 ActuatorStatus per Message
    const json jPayload(actuatorStatuses);
    const std::string topic = Channel::ACTUATION_STATUS_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                              Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER +
                              Channel::REFERENCE_PATH_PREFIX + Channel::CHANNEL_DELIMITER +
                              actuatorStatuses.getReference();
    const std::string payload = jPayload.dump();

    return std::make_shared<Message>(payload, topic);
}

bool JsonSingleProtocol::fromMessage(std::shared_ptr<Message> message, ActuatorSetCommand& command)
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

        const auto reference = referenceFromTopic(message->getChannel());

        command = ActuatorSetCommand(reference, value);
    }
    catch (...)
    {
        LOG(DEBUG) << "Unable to parse ActuatorSetCommand: " << message->getContent();
        return false;
    }

    return true;
}

bool JsonSingleProtocol::fromMessage(std::shared_ptr<Message> message, ActuatorGetCommand& command)
{
    try
    {
        const auto reference = referenceFromTopic(message->getChannel());

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

bool JsonSingleProtocol::isGatewayToPlatformMessage(const std::string& topic)
{
    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 6)
    {
        LOG(DEBUG) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::DEVICE_TO_PLATFORM_DIRECTION)
    {
        LOG(DEBUG) << "Device message dirrection not valid: " << topic;
        return false;
    }

    if (std::find(m_deviceMessageTypes.begin(), m_deviceMessageTypes.end(), tokens[TYPE_POS]) ==
        m_deviceMessageTypes.end())
    {
        LOG(DEBUG) << "Device message type not supported: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(DEBUG) << "Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_REFERENCE_TYPE_POS] != Channel::REFERENCE_PATH_PREFIX)
    {
        LOG(DEBUG) << "Reference perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool JsonSingleProtocol::isPlatformToGatewayMessage(const std::string& topic)
{
    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 6)
    {
        LOG(DEBUG) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::PLATFORM_TO_DEVICE_DIRECTION)
    {
        LOG(DEBUG) << "Device message dirrection not valid: " << topic;
        return false;
    }

    if (std::find(m_platformMessageTypes.begin(), m_platformMessageTypes.end(), tokens[TYPE_POS]) ==
        m_platformMessageTypes.end())
    {
        LOG(DEBUG) << "Device message type not supported: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(DEBUG) << "Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_REFERENCE_TYPE_POS] != Channel::REFERENCE_PATH_PREFIX)
    {
        LOG(DEBUG) << "Device perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool JsonSingleProtocol::isDeviceToPlatformMessage(const std::string& topic)
{
    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 6)
    {
        LOG(DEBUG) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::DEVICE_TO_PLATFORM_DIRECTION)
    {
        LOG(DEBUG) << "Device message dirrection not valid: " << topic;
        return false;
    }

    if (std::find(m_deviceMessageTypes.begin(), m_deviceMessageTypes.end(), tokens[TYPE_POS]) ==
        m_deviceMessageTypes.end())
    {
        LOG(DEBUG) << "Device message type not supported: " << topic;
        return false;
    }

    if (tokens[DEVICE_TYPE_POS] != Channel::DEVICE_PATH_PREFIX)
    {
        LOG(DEBUG) << "Device perfix missing in path: " << topic;
        return false;
    }

    if (tokens[DEVICE_REFERENCE_TYPE_POS] != Channel::REFERENCE_PATH_PREFIX)
    {
        LOG(DEBUG) << "Reference perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool JsonSingleProtocol::isPlatformToDeviceMessage(const std::string& topic)
{
    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 8)
    {
        LOG(DEBUG) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::PLATFORM_TO_DEVICE_DIRECTION)
    {
        LOG(DEBUG) << "Device message dirrection not valid: " << topic;
        return false;
    }

    if (std::find(m_platformMessageTypes.begin(), m_platformMessageTypes.end(), tokens[TYPE_POS]) ==
        m_platformMessageTypes.end())
    {
        LOG(DEBUG) << "Device message type not supported: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(DEBUG) << "Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_DEVICE_TYPE_POS] != Channel::DEVICE_PATH_PREFIX)
    {
        LOG(DEBUG) << "Device perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_DEVICE_REFERENCE_TYPE_POS] != Channel::REFERENCE_PATH_PREFIX)
    {
        LOG(DEBUG) << "Reference perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool JsonSingleProtocol::isActuatorSetMessage(const std::string& topic)
{
    return StringUtils::startsWith(topic, Channel::ACTUATION_SET_TOPIC_ROOT);
}

bool JsonSingleProtocol::isActuatorGetMessage(const std::string& topic)
{
    return StringUtils::startsWith(topic, Channel::ACTUATION_GET_TOPIC_ROOT);
}

std::string JsonSingleProtocol::routePlatformMessage(const std::string& topic, const std::string& gatewayKey)
{
    const std::string gwTopicPart =
      Channel::GATEWAY_PATH_PREFIX + Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER;
    if (topic.find(gwTopicPart) != std::string::npos)
    {
        return StringUtils::removeSubstring(topic, gwTopicPart);
    }

    return "";
}

std::string JsonSingleProtocol::routeDeviceMessage(const std::string& topic, const std::string& gatewayKey)
{
    auto firstPos = topic.find(Channel::CHANNEL_DELIMITER);
    if (firstPos == std::string::npos)
    {
        return "";
    }

    auto secondPos = topic.find(Channel::CHANNEL_DELIMITER, firstPos + Channel::CHANNEL_DELIMITER.length());
    if (secondPos == std::string::npos)
    {
        return "";
    }

    std::string newTopic = topic;
    return newTopic.insert(
      secondPos + Channel::CHANNEL_DELIMITER.length(),
      Channel::GATEWAY_PATH_PREFIX + Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER);
}

std::string JsonSingleProtocol::referenceFromTopic(const std::string& topic)
{
    std::string top{topic};

    if (top.back() == '/')
    {
        top.pop_back();
    }

    auto pos = top.rfind("/r/");

    if (pos != std::string::npos)
    {
        return top.substr(pos + 3, std::string::npos);
    }

    return "";
}

std::string JsonSingleProtocol::deviceKeyFromTopic(const std::string& topic)
{
    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() < GATEWAY_KEY_POS + 1 || tokens.size() < DEVICE_KEY_POS + 1)
    {
        LOG(DEBUG) << "Token count mismatch in path: " << topic;
        return "";
    }

    if (tokens.size() > GATEWAY_DEVICE_KEY_POS)
    {
        if (tokens[GATEWAY_TYPE_POS] == Channel::GATEWAY_PATH_PREFIX &&
            tokens[GATEWAY_DEVICE_TYPE_POS] == Channel::DEVICE_PATH_PREFIX)
        {
            return tokens[GATEWAY_DEVICE_KEY_POS];
        }
    }

    if (tokens[GATEWAY_TYPE_POS] == Channel::GATEWAY_PATH_PREFIX)
    {
        return tokens[GATEWAY_KEY_POS];
    }
    else if (tokens[DEVICE_TYPE_POS] == Channel::DEVICE_PATH_PREFIX)
    {
        return tokens[DEVICE_KEY_POS];
    }

    return "";
}
}    // namespace wolkabout
