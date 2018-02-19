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

#ifndef INBOUNDWOLKABOUTMESSAGEHANDLER_H
#define INBOUNDWOLKABOUTMESSAGEHANDLER_H

#include "InboundMessageHandler.h"
#include "model/Message.h"
#include "utilities/CommandBuffer.h"

#include <string>
#include <vector>

namespace wolkabout
{

class InboundWolkaboutMessageHandler: public InboundMessageHandler
{
public:
	InboundWolkaboutMessageHandler(const std::string& gatewayKey);

	void messageReceived(const std::string& topic, const std::string& message) override;

	const std::vector<std::string>& getTopics() const override;

	void setActuatorSetRequestHandler(std::function<void(Message)> handler);
	void setActuatorGetRequestHandler(std::function<void(Message)> handler);
	void setConfigurationSetRequestHandler(std::function<void(Message)> handler);
	void setConfigurationGetRequestHandler(std::function<void(Message)> handler);
	void setDeviceRegistrationResponseHandler(std::function<void(Message)> handler);
	void setDeviceReregistrationRequestHandler(std::function<void(Message)> handler);

//	void setBinaryDataHandler(std::function<void(BinaryData)> handler);

//	void setFirmwareUpdateCommandHandler(std::function<void(FirmwareUpdateCommand)> handler);

private:
	void addToCommandBuffer(std::function<void()> command);

	std::unique_ptr<CommandBuffer> m_commandBuffer;
	const std::string m_gatewayKey;

	std::vector<std::string> m_subscriptionList;

	std::function<void(Message)> m_actuationSetHandler;
	std::function<void(Message)> m_actuationGetHandler;
	std::function<void(Message)> m_configurationSetHandler;
	std::function<void(Message)> m_configurationGetHandler;
	std::function<void(Message)> m_deviceRegistrationResponseHandler;
	std::function<void(Message)> m_deviceReregistrationResuestHandler;
//	std::function<void(BinaryData)> m_binaryDataHandler;
//	std::function<void(FirmwareUpdateCommand)> m_firmwareUpdateHandler;

//	static const constexpr char* FIRMWARE_UPDATE_TOPIC_ROOT = "service/commands/firmware/";
//	static const constexpr char* BINARY_TOPIC_ROOT = "service/binary/";
};
}

#endif // INBOUNDWOLKABOUTMESSAGEHANDLER_H
