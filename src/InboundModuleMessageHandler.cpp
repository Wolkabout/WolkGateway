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

namespace wolkabout
{
InboundModuleMessageHandler::InboundModuleMessageHandler() :
	m_commandBuffer{new CommandBuffer()}
{
}

void InboundModuleMessageHandler::messageReceived(const std::string& topic, const std::string& message)
{
	LOG(DEBUG) << "Message received: " << topic << ", " << message;

	if(StringUtils::startsWith(topic, ACTUATION_STATUS_TOPIC_ROOT))
	{
		const size_t referencePosition = topic.find_last_of('/');
		if (referencePosition == std::string::npos)
		{
			return;
		}

		ActuatorStatus actuatorStatus;
//		if (!JsonParser::fromJson(message, actuatorCommand))
//		{
//			return;
//		}

		const std::string reference = topic.substr(referencePosition + 1);

		addToCommandBuffer([=]() -> void {
			if(m_actuationStatusHandler)
			{
				m_actuationStatusHandler(ActuatorStatus(reference, actuatorStatus.getValue(), actuatorStatus.getState()));
			}
		});
	}
}

const std::vector<std::string>& InboundModuleMessageHandler::getTopics() const
{
	return m_subscriptionList;
}

void InboundModuleMessageHandler::setActuatorStatusHandler(std::function<void(ActuatorStatus)> handler)
{
	m_actuationStatusHandler = handler;
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
