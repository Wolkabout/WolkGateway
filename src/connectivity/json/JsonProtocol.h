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

#ifndef JSONPROTOCOL_H
#define JSONPROTOCOL_H

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class SensorReading;
class Alarm;
class ActuatorStatus;
class ActuatorSetCommand;
class ActuatorGetCommand;

class JsonProtocol
{
public:
    ~JsonProtocol() = delete;

    static const std::string& getName();

    static const std::vector<std::string>& getDeviceChannels();
    static const std::vector<std::string>& getPlatformChannels();

    static std::shared_ptr<Message> make(const std::string& gatewayKey, std::shared_ptr<ActuatorStatus> actuatorStatus);
    static std::shared_ptr<Message> make(const std::string& gatewayKey, const ActuatorStatus& actuatorStatus);

    static bool fromMessage(std::shared_ptr<Message> message, ActuatorSetCommand& command);

    static bool fromMessage(std::shared_ptr<Message> message, ActuatorGetCommand& command);

    static bool isMessageToPlatform(const std::string& channel);
    static bool isMessageFromPlatform(const std::string& channel);

    static bool isActuatorSetMessage(const std::string& channel);
    static bool isActuatorGetMessage(const std::string& channel);

    static bool isSensorReadingMessage(const std::string& channel);
    static bool isAlarmMessage(const std::string& channel);
    static bool isActuatorStatusMessage(const std::string& channel);

    static std::string routePlatformToDeviceMessage(const std::string& topic, const std::string& gatewayKey);
    static std::string routeDeviceToPlatformMessage(const std::string& topic, const std::string& gatewayKey);

    static std::string routePlatformToGatewayMessage(const std::string& topic);
    static std::string routeGatewayToPlatformMessage(const std::string& topic);

    static std::string extractReferenceFromChannel(const std::string& topic);
    static std::string extractDeviceKeyFromChannel(const std::string& topic);

private:
    static const std::string NAME;

    static const std::string CHANNEL_DELIMITER;
    static const std::string CHANNEL_WILDCARD;

    static const std::string GATEWAY_TYPE;
    static const std::string DEVICE_TYPE;
    static const std::string REFERENCE_TYPE;
    static const std::string DEVICE_TO_PLATFORM_TYPE;
    static const std::string PLATFORM_TO_DEVICE_TYPE;

    static const std::string GATEWAY_PATH_PREFIX;
    static const std::string DEVICE_PATH_PREFIX;
    static const std::string REFERENCE_PATH_PREFIX;
    static const std::string DEVICE_TO_PLATFORM_DIRECTION;
    static const std::string PLATFORM_TO_DEVICE_DIRECTION;

    static const std::string SENSOR_READING_TOPIC_ROOT;
    static const std::string EVENTS_TOPIC_ROOT;
    static const std::string ACTUATION_STATUS_TOPIC_ROOT;
    static const std::string CONFIGURATION_SET_RESPONSE_TOPIC_ROOT;
    static const std::string CONFIGURATION_GET_RESPONSE_TOPIC_ROOT;

    static const std::string ACTUATION_SET_TOPIC_ROOT;
    static const std::string ACTUATION_GET_TOPIC_ROOT;
    static const std::string CONFIGURATION_SET_REQUEST_TOPIC_ROOT;
    static const std::string CONFIGURATION_GET_REQUEST_TOPIC_ROOT;

    static const std::vector<std::string> DEVICE_CHANNELS;
    static const std::vector<std::string> PLATFORM_CHANNELS;
};
}    // namespace wolkabout

#endif
