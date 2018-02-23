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
#include "utilities/json.hpp"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "model/Message.h"
#include "model/ActuatorSetCommand.h"
#include "model/ActuatorGetCommand.h"
#include "model/SensorReading.h"
#include "model/Alarm.h"
#include "model/ActuatorStatus.h"

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
	if(!p)
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
	if(!p)
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
	if(!p)
	{
		return;
	}

	to_json(j, *p);
}

JsonSingleProtocol::JsonSingleProtocol()
	: m_devicTopics{Channel::SENSOR_READING_TOPIC_ROOT + Channel::CHANNEL_WILDCARD,
					Channel::EVENTS_TOPIC_ROOT + Channel::CHANNEL_WILDCARD,
					Channel::ACTUATION_STATUS_TOPIC_ROOT + Channel::CHANNEL_WILDCARD},
	  m_platformTopics{Channel::ACTUATION_GET_TOPIC_ROOT + Channel::CHANNEL_WILDCARD,
					   Channel::ACTUATION_SET_TOPIC_ROOT + Channel::CHANNEL_WILDCARD}
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
	const std::string topic = Channel::SENSOR_READING_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + gatewayKey +
			Channel::CHANNEL_DELIMITER + Channel::REFERENCE_PATH_PREFIX + Channel::CHANNEL_DELIMITER +
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
	const std::string topic = Channel::EVENTS_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + gatewayKey + Channel::CHANNEL_DELIMITER +
			Channel::REFERENCE_PATH_PREFIX + Channel::CHANNEL_DELIMITER + alarms.front()->getReference();
	const std::string payload = jPayload.dump();

	return std::make_shared<Message>(payload, topic);
}

std::shared_ptr<Message> JsonSingleProtocol::make(const std::string& gatewayKey,
												  std::vector<std::shared_ptr<ActuatorStatus>> actuatorStatuses)
{
	if (actuatorStatuses.size() == 0)
	{
		return nullptr;
	}

	/* JSON_SINGLE allows only 1 ActuatorStatus per Message, hence only first element is deserialized*/
	const json jPayload(actuatorStatuses.front());
	const std::string topic = Channel::ACTUATION_STATUS_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + gatewayKey +
			Channel::CHANNEL_DELIMITER + Channel::REFERENCE_PATH_PREFIX + Channel::CHANNEL_DELIMITER +
			actuatorStatuses.front()->getReference();
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

		const auto reference = referenceFromTopic(message->getTopic());

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
		const auto reference = referenceFromTopic(message->getTopic());

		command = ActuatorGetCommand(reference);
	}
	catch (...)
	{
		LOG(DEBUG) << "Unable to parse ActuatorGetCommand: " << message->getContent();
		return false;
	}

	return true;
}

bool JsonSingleProtocol::isGatewayMessage(const std::string& topic)
{
	auto firstPos = topic.find(Channel::CHANNEL_DELIMITER);
	if(firstPos == std::string::npos)
	{
		return false;
	}

	auto secondPos = topic.find(Channel::CHANNEL_DELIMITER, firstPos + 1);
	if(secondPos == std::string::npos)
	{
		return false;
	}

	return topic.find(Channel::GATEWAY_PATH_PREFIX, secondPos + 1) == secondPos + 1;
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
	const std::string gwTopicPart = Channel::GATEWAY_PATH_PREFIX + gatewayKey + Channel::CHANNEL_DELIMITER;
	if(topic.find(gwTopicPart) != std::string::npos)
	{
		return StringUtils::removeSubstring(topic, gwTopicPart);
	}

	return "";
}

std::string JsonSingleProtocol::routeDeviceMessage(const std::string& topic, const std::string& gatewayKey)
{
	auto firstPos = topic.find(Channel::CHANNEL_DELIMITER);
	if(firstPos == std::string::npos)
	{
		return "";
	}

	auto secondPos = topic.find(Channel::CHANNEL_DELIMITER, firstPos + 1);
	if(secondPos == std::string::npos)
	{
		return "";
	}

	std::string newTopic = topic;
	return newTopic.insert(secondPos + Channel::CHANNEL_DELIMITER.length(),
						   Channel::GATEWAY_PATH_PREFIX + gatewayKey + Channel::CHANNEL_DELIMITER);
}

std::string JsonSingleProtocol::referenceFromTopic(std::string topic)
{
	if(topic.back() == '/')
	{
		topic.pop_back();
	}

	auto pos = topic.rfind("/r/");

	if(pos != std::string::npos)
	{
		return topic.substr(pos + 3, std::string::npos);
	}

	return "";
}
}
