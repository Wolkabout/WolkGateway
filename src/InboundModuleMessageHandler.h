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

#ifndef INBOUNDMODULEMESSAGEHANDLER_H
#define INBOUNDMODULEMESSAGEHANDLER_H

#include "connectivity/ConnectivityService.h"
#include "model/DeviceManifest.h"
#include "model/ActuatorStatus.h"
#include "utilities/CommandBuffer.h"

#include <string>
#include <map>
#include <vector>

namespace wolkabout
{
class InboundModuleMessageHandler: public ConnectivityServiceListener
{
	InboundModuleMessageHandler();

	void messageReceived(const std::string& topic, const std::string& message) override;

	const std::vector<std::string>& getTopics() const override;

	void setActuatorStatusHandler(std::function<void(ActuatorStatus)> handler);

//	void setBinaryDataHandler(std::function<void(BinaryData)> handler);

//	void setFirmwareUpdateCommandHandler(std::function<void(FirmwareUpdateCommand)> handler);

private:
	void addToCommandBuffer(std::function<void()> command);

	std::unique_ptr<CommandBuffer> m_commandBuffer;

	std::map<std::string, DeviceManifest> m_devices;
	std::vector<std::string> m_subscriptionList;

	std::function<void(ActuatorStatus)> m_actuationStatusHandler;
//	std::function<void(BinaryData)> m_binaryDataHandler;
//	std::function<void(FirmwareUpdateCommand)> m_firmwareUpdateHandler;

	static const constexpr char* SENSOR_READING_TOPIC_ROOT = "d2p/sensor_reading/";
	static const constexpr char* ACTUATION_STATUS_TOPIC_ROOT = "d2p/actuator_status/";
	static const constexpr char* CONFIGURATION_TOPIC_ROOT = "d2p/configuration_get/";
	static const constexpr char* DEVICE_STATUS_ROOT = "d2p/status/";
	static const constexpr char* DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT = "d2p/register_device/";
	static const constexpr char* DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT = "d2p/reregister_device/";
};
}

#endif
