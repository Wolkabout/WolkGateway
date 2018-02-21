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

#include "InboundModuleMessageHandler.h"
#include "utilities/StringUtils.h"
#include "utilities/Logger.h"
#include "connectivity/Channels.h"

namespace wolkabout
{
InboundModuleMessageHandler::InboundModuleMessageHandler() :
	m_commandBuffer{new CommandBuffer()}
{
	std::string topic = Channel::SENSOR_READING_TOPIC_ROOT + Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::EVENTS_TOPIC_ROOT + Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::ACTUATION_STATUS_TOPIC_ROOT + Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::CONFIGURATION_GET_RESPONSE_TOPIC_ROOT + Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::CONFIGURATION_SET_RESPONSE_TOPIC_ROOT + Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::DEVICE_STATUS_TOPIC_ROOT + Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT + Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);
}

void InboundModuleMessageHandler::messageReceived(const std::string& topic, const std::string& message)
{
	LOG(DEBUG) << "Module message received: " << topic << ", " << message;

	if(StringUtils::startsWith(topic, Channel::SENSOR_READING_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_sensorReadingHandler)
			{
				Message msg{message, topic};
				m_sensorReadingHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::EVENTS_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_alarmHandler)
			{
				Message msg{message, topic};
				m_alarmHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::ACTUATION_STATUS_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_actuationStatusHandler)
			{
				Message msg{message, topic};
				m_actuationStatusHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::CONFIGURATION_GET_RESPONSE_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_configurationHandler)
			{
				Message msg{message, topic};
				m_configurationHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::CONFIGURATION_SET_RESPONSE_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_configurationHandler)
			{
				Message msg{message, topic};
				m_configurationHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::DEVICE_STATUS_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_deviceStatusHandler)
			{
				Message msg{message, topic};
				m_deviceStatusHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_deviceRegistrationRequestHandler)
			{
				Message msg{message, topic};
				m_deviceRegistrationRequestHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_deviceReregistrationResponseHandler)
			{
				Message msg{message, topic};
				m_deviceReregistrationResponseHandler(msg);
			}
		});
	}
	else
	{
		LOG(WARN) << "Unable to parse module message: " << topic << ", " << message;
	}
}

const std::vector<std::string>& InboundModuleMessageHandler::getTopics() const
{
	return m_subscriptionList;
}

void InboundModuleMessageHandler::setSensorReadingHandler(std::function<void(Message)> handler)
{
	m_sensorReadingHandler = handler;
}

void InboundModuleMessageHandler::setAlarmHandler(std::function<void(Message)> handler)
{
	m_alarmHandler = handler;
}

void InboundModuleMessageHandler::setActuatorStatusHandler(std::function<void(Message)> handler)
{
	m_actuationStatusHandler = handler;
}

void InboundModuleMessageHandler::setConfigurationHandler(std::function<void(Message)> handler)
{
	m_configurationHandler = handler;
}

void InboundModuleMessageHandler::setDeviceStatusHandler(std::function<void(Message)> handler)
{
	m_deviceStatusHandler = handler;
}

void InboundModuleMessageHandler::setDeviceRegistrationRequestHandler(std::function<void(Message)> handler)
{
	m_deviceRegistrationRequestHandler = handler;
}

void InboundModuleMessageHandler::setDeviceReregistrationResponseHandler(std::function<void(Message)> handler)
{
	m_deviceReregistrationResponseHandler = handler;
}

//void InboundMessageHandler::setBinaryDataHandler(std::function<void(BinaryData)> handler)
//{
//	m_binaryDataHandler = handler;
//}

//void InboundMessageHandler::setFirmwareUpdateCommandHandler(std::function<void(FirmwareUpdateCommand)> handler)
//{
//	m_firmwareUpdateHandler = handler;
//}

void InboundModuleMessageHandler::addToCommandBuffer(std::function<void()> command)
{
	m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(command));
}

}
