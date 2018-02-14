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
#include "utilities/Logger.h"

#include <sstream>
#include <iostream>

namespace wolkabout
{

InboundWolkaboutMessageHandler::InboundWolkaboutMessageHandler(const std::string& gatewayKey) :
	m_commandBuffer{new CommandBuffer()}, m_gatewayKey{gatewayKey}
{
}

void InboundWolkaboutMessageHandler::messageReceived(const std::string& topic, const std::string& message)
{
	LOG(DEBUG) << "Message received: " << topic << ", " << message;

	if(StringUtils::startsWith(topic, ACTUATION_SET_REQUEST_TOPIC_ROOT))
	{
		const size_t referencePosition = topic.find_last_of('/');
		if (referencePosition == std::string::npos)
		{
			return;
		}

		ActuatorSetCommand actuatorCommand;
//		if (!JsonParser::fromJson(message, actuatorCommand))
//		{
//			return;
//		}

		const std::string reference = topic.substr(referencePosition + 1);

		addToCommandBuffer([=]() -> void {
			if(m_actuationSetHandler)
			{
				m_actuationSetHandler(ActuatorSetCommand(reference, actuatorCommand.getValue()));
			}
		});
	}
//	else if(StringUtils::startsWith(topic, FIRMWARE_UPDATE_TOPIC_ROOT))
//	{
//		FirmwareUpdateCommand firmwareUpdateCommand;
//		if (!JsonParser::fromJson(message, firmwareUpdateCommand))
//		{
//			return;
//		}

//		addToCommandBuffer([=]() -> void {
//			if(m_firmwareUpdateHandler)
//			{
//				m_firmwareUpdateHandler(firmwareUpdateCommand);
//			}
//		});
//	}
//	else if(StringUtils::startsWith(topic, BINARY_TOPIC_ROOT))
//	{
//		try
//		{
//			BinaryData data{ByteUtils::toByteArray(message)};

//			addToCommandBuffer([=]() -> void {
//				if(m_binaryDataHandler)
//				{
//					m_binaryDataHandler(data);
//				}
//			});
//		}
//		catch (const std::invalid_argument& e)
//		{
//			std::cout << e.what();
//		}
//	}
}

const std::vector<std::string>& InboundWolkaboutMessageHandler::getTopics() const
{
	return m_subscriptionList;
}

void InboundWolkaboutMessageHandler::setActuatorSetCommandHandler(std::function<void(ActuatorSetCommand)> handler)
{
	m_actuationSetHandler = handler;
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
