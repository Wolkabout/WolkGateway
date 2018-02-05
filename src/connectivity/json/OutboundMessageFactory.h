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

#ifndef OUTBOUNDMESSAGEFACTORY_H
#define OUTBOUNDMESSAGEFACTORY_H

#include "model/OutboundMessage.h"

#include <memory>
#include <vector>

namespace wolkabout
{
class ActuatorStatus;
class Alarm;
class SensorReading;
class FirmwareUpdateResponse;
class FilePacketRequest;

class OutboundMessageFactory
{
public:
    OutboundMessageFactory() = delete;

    static std::shared_ptr<OutboundMessage> make(const std::string& deviceKey,
                                                 std::vector<std::shared_ptr<SensorReading>> sensorReadings);
    static std::shared_ptr<OutboundMessage> make(const std::string& deviceKey,
                                                 std::vector<std::shared_ptr<Alarm>> alarms);
    static std::shared_ptr<OutboundMessage> make(const std::string& deviceKey,
                                                 std::vector<std::shared_ptr<ActuatorStatus>> actuatorStatuses);

	static std::shared_ptr<OutboundMessage> make(const std::string& deviceKey,
												 const FirmwareUpdateResponse& firmwareUpdateResponse);

	static std::shared_ptr<OutboundMessage> make(const std::string& deviceKey,
												 const FilePacketRequest& filePacketRequest);

	static std::shared_ptr<OutboundMessage> makeFromFirmwareVersion(const std::string& deviceKey,
																	const std::string& firmwareVerion);

private:
    static const constexpr char* SENSOR_READINGS_TOPIC_ROOT = "readings/";
    static const constexpr char* ALARMS_TOPIC_ROOT = "events/";
	static const constexpr char* ACTUATOR_STATUS_TOPIC_ROOT = "actuators/status/";
	static const constexpr char* FIRMWARE_UPDATE_STATUS_TOPIC_ROOT = "service/status/firmware/";
	static const constexpr char* FILE_HANDLING_STATUS_TOPIC_ROOT = "service/status/file/";
	static const constexpr char* FIRMWARE_VERSION_TOPIC_ROOT = "firmware/version/";
};
}

#endif
