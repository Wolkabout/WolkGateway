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

#include "connectivity/ConnectivityService.h"
#include "model/Device.h"
#include "model/ActuatorSetCommand.h"
#include "model/BinaryData.h"
#include "model/FirmwareUpdateCommand.h"
#include "model/DeviceManifest.h"
#include "utilities/CommandBuffer.h"

#include <string>
#include <vector>
#include <map>

namespace wolkabout
{

class InboundWolkaboutMessageHandler: public ConnectivityServiceListener
{
public:
	InboundWolkaboutMessageHandler(const std::string& gatewayKey);

	void messageReceived(const std::string& topic, const std::string& message) override;

	const std::vector<std::string>& getTopics() const override;

	void setActuatorSetCommandHandler(std::function<void(ActuatorSetCommand)> handler);

//	void setBinaryDataHandler(std::function<void(BinaryData)> handler);

//	void setFirmwareUpdateCommandHandler(std::function<void(FirmwareUpdateCommand)> handler);

private:
	void addToCommandBuffer(std::function<void()> command);

	std::unique_ptr<CommandBuffer> m_commandBuffer;
	const std::string m_gatewayKey;

	std::map<std::string, DeviceManifest> m_devices;
	std::vector<std::string> m_subscriptionList;

	std::function<void(ActuatorSetCommand)> m_actuationSetHandler;
//	std::function<void(BinaryData)> m_binaryDataHandler;
//	std::function<void(FirmwareUpdateCommand)> m_firmwareUpdateHandler;

	static const constexpr char* ACTUATION_SET_REQUEST_TOPIC_ROOT = "p2d/actuator_set/";
	static const constexpr char* ACTUATION_GET_REQUEST_TOPIC_ROOT = "p2d/actuator_get/";
	static const constexpr char* CONFIGURATION_SET_REQUEST_TOPIC_ROOT = "p2d/configuration_set/";
	static const constexpr char* CONFIGURATION_GET_REQUEST_TOPIC_ROOT = "p2d/configuration_get/";
	static const constexpr char* DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT = "p2d/register_device/";
	static const constexpr char* DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT = "p2d/reregister_device/";

//	static const constexpr char* FIRMWARE_UPDATE_TOPIC_ROOT = "service/commands/firmware/";
//	static const constexpr char* BINARY_TOPIC_ROOT = "service/binary/";
};
}

#endif // INBOUNDWOLKABOUTMESSAGEHANDLER_H
