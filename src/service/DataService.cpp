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

#include "DataService.h"
#include "utilities/StringUtils.h"
#include "model/SensorReading.h"
#include "model/Alarm.h"
#include "model/ActuatorStatus.h"
#include "model/ActuatorSetCommand.h"
#include "model/ActuatorGetCommand.h"
#include "connectivity/MessageFactory.h"
#include "OutboundMessageHandler.h"
#include "ActuatorCommandListener.h"
#include "connectivity/Channels.h"
#include "utilities/Logger.h"
#include <sstream>

namespace wolkabout
{
DataService::DataService(const std::string& gatewayKey, std::unique_ptr<MessageFactory> protocol,
						 std::shared_ptr<OutboundMessageHandler> outboundWolkaboutMessageHandler,
						 std::shared_ptr<OutboundMessageHandler> outboundModuleMessageHandler,
						 std::weak_ptr<ActuatorCommandListener> actuationHandler) :
	m_gatewayKey{gatewayKey},
	m_protocol{std::move(protocol)},
	m_outboundWolkaboutMessageHandler{std::move(outboundWolkaboutMessageHandler)},
	m_outboundModuleMessageHandler{std::move(outboundModuleMessageHandler)},
	m_actuationHandler{actuationHandler}
{
}

void DataService::handleSensorReading(Message reading)
{
	routeModuleMessage(reading, Channel::SENSOR_READING_TOPIC_ROOT);
}

void DataService::handleAlarm(Message alarm)
{
	routeModuleMessage(alarm, Channel::EVENTS_TOPIC_ROOT);
}

void DataService::handleActuatorSetCommand(Message command)
{
	const std::string topicGwPart = Channel::ACTUATION_SET_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER;

	if(!StringUtils::startsWith(command.getTopic(),  topicGwPart))
	{
		LOG(DEBUG) << "Invalid message path: " << command.getTopic();
		return;
	}

	const std::string referencePath = StringUtils::removeSubstring(command.getTopic(), topicGwPart);

	if(StringUtils::startsWith(referencePath, Channel::DEVICE_PATH_PREFIX))
	{
		routeWolkaboutMessage(command);
	}
	else
	{
		ActuatorSetCommand cmd;
		if(!m_protocol->fromJson(command.getContent(), cmd))
		{
			LOG(DEBUG) << "Actuation message could not be parsed: " << command.getContent();
		}

		if(auto handler = m_actuationHandler.lock())
		{
			handler->handleActuatorSetCommand(ActuatorSetCommand{referencePath, cmd.getValue()});
		}
	}
}

void DataService::handleActuatorGetCommand(Message command)
{
	const std::string topicGwPart = Channel::ACTUATION_SET_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER;

	if(!StringUtils::startsWith(command.getTopic(), topicGwPart))
	{
		LOG(DEBUG) << "Invalid message path: " << command.getTopic();
		return;
	}

	const std::string referencePath = StringUtils::removeSubstring(command.getTopic(), topicGwPart);

	if(StringUtils::startsWith(referencePath, Channel::DEVICE_PATH_PREFIX))
	{
		routeWolkaboutMessage(command);
	}
	else
	{
		ActuatorGetCommand cmd{referencePath};

		if(auto handler = m_actuationHandler.lock())
		{
			handler->handleActuatorGetCommand(cmd);
		}
	}
}

void DataService::handleActuatorStatus(Message status)
{
	routeModuleMessage(status, Channel::ACTUATION_STATUS_TOPIC_ROOT);
}

void DataService::addSensorReadings(std::vector<std::shared_ptr<SensorReading>> sensorReadings)
{
	if(sensorReadings.size() == 0)
	{
		return;
	}

	std::string topic = Channel::SENSOR_READING_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER +
			sensorReadings.front()->getReference();

	const std::shared_ptr<Message> outboundMessage = m_protocol->make(topic, sensorReadings);

	if(!outboundMessage)
	{
		LOG(WARN) << "Message not created for " << topic;
		return;
	}

	m_outboundWolkaboutMessageHandler->addMessage(outboundMessage);
}

void DataService::addAlarms(std::vector<std::shared_ptr<Alarm>> alarms)
{
	if(alarms.size() == 0)
	{
		return;
	}

	std::string topic = Channel::EVENTS_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER +
			alarms.front()->getReference();

	const std::shared_ptr<Message> outboundMessage = m_protocol->make(topic, alarms);

	if(!outboundMessage)
	{
		LOG(WARN) << "Message not created for " << topic;
		return;
	}

	m_outboundWolkaboutMessageHandler->addMessage(outboundMessage);
}

void DataService::addActuatorStatus(std::shared_ptr<ActuatorStatus> actuatorStatus)
{
	std::string topic = Channel::ACTUATION_STATUS_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER +
			actuatorStatus->getReference();

	const std::shared_ptr<Message> outboundMessage = m_protocol->make(topic, {actuatorStatus});

	if(!outboundMessage)
	{
		LOG(WARN) << "Message not created for " << topic;
		return;
	}

	m_outboundWolkaboutMessageHandler->addMessage(outboundMessage);
}

void DataService::routeModuleMessage(const Message& message, const std::string& topicRoot)
{
	const std::string topicRefPath = StringUtils::removeSubstring(message.getTopic(), topicRoot);

	std::string routedPath = topicRoot + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER + topicRefPath;

	const std::shared_ptr<Message> routedMessage{new Message(message.getContent(), routedPath)};

	m_outboundWolkaboutMessageHandler->addMessage(routedMessage);
}

void DataService::routeWolkaboutMessage(const Message& message)
{
	const std::string gwPathPart = Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER;
	const std::string routedPath = StringUtils::removeSubstring(message.getTopic(), gwPathPart);

	const std::shared_ptr<Message> routedMessage{new Message(message.getContent(), routedPath)};

	m_outboundModuleMessageHandler->addMessage(routedMessage);
}

}
