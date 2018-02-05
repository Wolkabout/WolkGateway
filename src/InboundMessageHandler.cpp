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

#include "InboundMessageHandler.h"
#include "connectivity/json/JsonDtoParser.h"
#include "utilities/StringUtils.h"
#include "utilities/ByteUtils.h"

#include <sstream>
#include <iostream>

namespace wolkabout
{

InboundMessageHandler::InboundMessageHandler(Device device) :
	m_device{device}, m_commandBuffer{new CommandBuffer()}
{
	for (const std::string& actuatorReference : m_device.getActuatorReferences())
	{
		std::stringstream topic("");
		topic << ACTUATION_REQUEST_TOPIC_ROOT << m_device.getDeviceKey() << "/" << actuatorReference;
		m_subscriptionList.emplace_back(topic.str());
	}

	// firmware update
	std::stringstream topic("");
	topic << FIRMWARE_UPDATE_TOPIC_ROOT << m_device.getDeviceKey();
	m_subscriptionList.emplace_back(topic.str());

	// file handling
	std::stringstream binaryTopic("");
	binaryTopic << BINARY_TOPIC_ROOT << m_device.getDeviceKey();
	m_subscriptionList.emplace_back(binaryTopic.str());
}

void InboundMessageHandler::messageReceived(const std::string& topic, const std::string& message)
{
	//std::cout << "Message received: " << topic << ", " << message << std::endl;

	if(StringUtils::startsWith(topic, ACTUATION_REQUEST_TOPIC_ROOT))
	{
		const size_t referencePosition = topic.find_last_of('/');
		if (referencePosition == std::string::npos)
		{
			return;
		}

		ActuatorCommand actuatorCommand;
		if (!JsonParser::fromJson(message, actuatorCommand))
		{
			return;
		}

		const std::string reference = topic.substr(referencePosition + 1);

		addToCommandBuffer([=]() -> void {
			if(m_actuationHandler)
			{
				m_actuationHandler(ActuatorCommand(actuatorCommand.getType(), reference, actuatorCommand.getValue()));
			}
		});
	}
	else if(StringUtils::startsWith(topic, FIRMWARE_UPDATE_TOPIC_ROOT))
	{
		FirmwareUpdateCommand firmwareUpdateCommand;
		if (!JsonParser::fromJson(message, firmwareUpdateCommand))
		{
			return;
		}

		addToCommandBuffer([=]() -> void {
			if(m_firmwareUpdateHandler)
			{
				m_firmwareUpdateHandler(firmwareUpdateCommand);
			}
		});
	}
	else if(StringUtils::startsWith(topic, BINARY_TOPIC_ROOT))
	{
		try
		{
			BinaryData data{ByteUtils::toByteArray(message)};

			addToCommandBuffer([=]() -> void {
				if(m_binaryDataHandler)
				{
					m_binaryDataHandler(data);
				}
			});
		}
		catch (const std::invalid_argument& e)
		{
			std::cout << e.what();
		}
	}
}

const std::vector<std::string>& InboundMessageHandler::getTopics() const
{
	return m_subscriptionList;
}

void InboundMessageHandler::setActuatorCommandHandler(std::function<void(ActuatorCommand)> handler)
{
	m_actuationHandler = handler;
}

void InboundMessageHandler::setBinaryDataHandler(std::function<void(BinaryData)> handler)
{
	m_binaryDataHandler = handler;
}

void InboundMessageHandler::setFirmwareUpdateCommandHandler(std::function<void(FirmwareUpdateCommand)> handler)
{
	m_firmwareUpdateHandler = handler;
}

void InboundMessageHandler::addToCommandBuffer(std::function<void()> command)
{
	m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(command));
}

}
