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

#ifndef GATEWAYDATAPROTOCOL_H
#define GATEWAYDATAPROTOCOL_H

#include "protocol/GatewayProtocol.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class ActuatorStatus;
class ActuatorSetCommand;
class ActuatorGetCommand;

class GatewayDataProtocol : public GatewayProtocol
{
public:
    virtual std::unique_ptr<Message> makeMessage(const std::string& deviceKey,
                                                 const ActuatorGetCommand& command) const = 0;

    virtual bool isSensorReadingMessage(const Message& message) const = 0;
    virtual bool isAlarmMessage(const Message& message) const = 0;
    virtual bool isActuatorStatusMessage(const Message& message) const = 0;
    virtual bool isConfigurationCurrentMessage(const Message& message) const = 0;

    virtual std::string routePlatformToDeviceMessage(const std::string& topic, const std::string& gatewayKey) const = 0;
    virtual std::string routeDeviceToPlatformMessage(const std::string& topic, const std::string& gatewayKey) const = 0;

    virtual std::string routePlatformToGatewayMessage(const std::string& topic) const = 0;
    virtual std::string routeGatewayToPlatformMessage(const std::string& topic) const = 0;

    virtual std::string extractReferenceFromChannel(const std::string& topic) const = 0;
};
}    // namespace wolkabout

#endif
