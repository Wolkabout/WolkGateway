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

#include "InboundWolkaboutMessageHandler.h"
#include "connectivity/Channels.h"
#include "utilities/StringUtils.h"
#include "utilities/Logger.h"

#include <sstream>

namespace wolkabout
{

InboundWolkaboutMessageHandler::InboundWolkaboutMessageHandler(const std::string& gatewayKey) :
	m_commandBuffer{new CommandBuffer()}, m_gatewayKey{gatewayKey}
{
	std::string topic = Channel::ACTUATION_SET_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER +
			Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::ACTUATION_GET_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER +
			Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::CONFIGURATION_SET_REQUEST_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER +
			Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::CONFIGURATION_GET_REQUEST_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER +
			Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER +
			Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);

	topic = Channel::DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX + m_gatewayKey + Channel::CHANNEL_DELIMITER +
			Channel::CHANNEL_WILDCARD;
	m_subscriptionList.emplace_back(topic);
}

void InboundWolkaboutMessageHandler::messageReceived(const std::string& topic, const std::string& message)
{
	LOG(DEBUG) << "Message received from Wolkabout: " << topic << ", " << message;

	if(StringUtils::startsWith(topic, Channel::ACTUATION_SET_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_actuationSetHandler)
			{
				Message msg{message, topic};
				m_actuationSetHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::ACTUATION_GET_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_actuationGetHandler)
			{
				Message msg{message, topic};
				m_actuationGetHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::CONFIGURATION_SET_REQUEST_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_configurationSetHandler)
			{
				Message msg{message, topic};
				m_configurationSetHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::CONFIGURATION_GET_REQUEST_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_configurationGetHandler)
			{
				Message msg{message, topic};
				m_configurationGetHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_deviceRegistrationResponseHandler)
			{
				Message msg{message, topic};
				m_deviceRegistrationResponseHandler(msg);
			}
		});
	}
	else if(StringUtils::startsWith(topic, Channel::DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT))
	{
		addToCommandBuffer([=]{
			if(m_deviceReregistrationResuestHandler)
			{
				Message msg{message, topic};
				m_deviceReregistrationResuestHandler(msg);
			}
		});
	}
	else
	{
		LOG(DEBUG) << "Unable to parse message: " << topic << ", " << message;
	}
}

const std::vector<std::string>& InboundWolkaboutMessageHandler::getTopics() const
{
	return m_subscriptionList;
}

void InboundWolkaboutMessageHandler::setActuatorSetRequestHandler(std::function<void(Message)> handler)
{
	m_actuationSetHandler = handler;
}

void InboundWolkaboutMessageHandler::setActuatorGetRequestHandler(std::function<void(Message)> handler)
{
	m_actuationGetHandler = handler;
}

void InboundWolkaboutMessageHandler::setConfigurationSetRequestHandler(std::function<void(Message)> handler)
{
	m_configurationSetHandler = handler;
}

void InboundWolkaboutMessageHandler::setConfigurationGetRequestHandler(std::function<void(Message)> handler)
{
	m_configurationGetHandler = handler;
}

void InboundWolkaboutMessageHandler::setDeviceRegistrationResponseHandler(std::function<void(Message)> handler)
{
	m_deviceRegistrationResponseHandler = handler;
}

void InboundWolkaboutMessageHandler::setDeviceReregistrationRequestHandler(std::function<void(Message)> handler)
{
	m_deviceReregistrationResuestHandler = handler;
}

//void InboundMessageHandler::setBinaryDataHandler(std::function<void(BinaryData)> handler)
//{
//	m_binaryDataHandler = handler;
//}

//void InboundMessageHandler::setFirmwareUpdateCommandHandler(std::function<void(FirmwareUpdateCommand)> handler)
//{
//	m_firmwareUpdateHandler = handler;
//}

void InboundWolkaboutMessageHandler::addToCommandBuffer(std::function<void()> command)
{
	m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(command));
}

}
