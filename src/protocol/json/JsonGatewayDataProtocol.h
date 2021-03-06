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

#ifndef JSONGATEWAYDATAPROTOCOL_H
#define JSONGATEWAYDATAPROTOCOL_H

#include "protocol/GatewayDataProtocol.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class Alarm;
class ActuatorStatus;
class ActuatorSetCommand;
class ActuatorGetCommand;

class JsonGatewayDataProtocol : public GatewayDataProtocol
{
public:
    std::vector<std::string> getInboundChannels() const override;
    std::vector<std::string> getInboundChannelsForDevice(const std::string& deviceKey) const override;
    std::string extractDeviceKeyFromChannel(const std::string& topic) const override;

    std::unique_ptr<Message> makeMessage(const std::string& deviceKey,
                                         const ActuatorGetCommand& command) const override;

    bool isSensorReadingMessage(const Message& message) const override;
    bool isAlarmMessage(const Message& message) const override;
    bool isActuatorStatusMessage(const Message& message) const override;
    bool isConfigurationCurrentMessage(const Message& message) const override;

    std::string routePlatformToDeviceMessage(const std::string& topic, const std::string& gatewayKey) const override;
    std::string routeDeviceToPlatformMessage(const std::string& topic, const std::string& gatewayKey) const override;
    std::string extractReferenceFromChannel(const std::string& topic) const override;

private:
    static const std::string SENSOR_READING_TOPIC_ROOT;
    static const std::string EVENTS_TOPIC_ROOT;
    static const std::string ACTUATION_STATUS_TOPIC_ROOT;
    static const std::string CONFIGURATION_RESPONSE_TOPIC_ROOT;

    static const std::string ACTUATION_SET_TOPIC_ROOT;
    static const std::string ACTUATION_GET_TOPIC_ROOT;
    static const std::string CONFIGURATION_SET_REQUEST_TOPIC_ROOT;
    static const std::string CONFIGURATION_GET_REQUEST_TOPIC_ROOT;
};
}    // namespace wolkabout

#endif
