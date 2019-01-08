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

#ifndef GATEWAYDEVICEREGISTRATIONPROTOCOL_H
#define GATEWAYDEVICEREGISTRATIONPROTOCOL_H

#include "protocol/GatewayProtocol.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class DeviceRegistrationRequest;
class DeviceRegistrationResponse;
class DeviceReregistrationResponse;

class GatewayDeviceRegistrationProtocol : public GatewayProtocol
{
public:
    GatewayProtocol::Type getType() const override final { return GatewayProtocol::Type::REGISTRATION; }

    virtual std::unique_ptr<Message> makeMessage(const std::string& gatewayKey, const std::string& deviceKey,
                                                 const DeviceRegistrationRequest& request) const = 0;

    virtual std::unique_ptr<Message> makeMessage(const std::string& deviceKey,
                                                 const DeviceRegistrationResponse& request) const = 0;

    virtual std::unique_ptr<Message> makeMessage(const std::string& gatewayKey, const std::string& deviceKey,
                                                 const DeviceRegistrationResponse& request) const = 0;

    virtual std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                                 const DeviceReregistrationResponse& response) const = 0;

    virtual std::unique_ptr<Message> makeDeviceReregistrationRequestForDevice() const = 0;
    virtual std::unique_ptr<Message> makeDeviceReregistrationRequestForGateway(const std::string& gatewayKey) const = 0;

    virtual std::unique_ptr<Message> makeDeviceDeletionRequestMessage(const std::string& gatewayKey,
                                                                      const std::string& deviceKey) const = 0;

    virtual std::unique_ptr<DeviceRegistrationRequest> makeRegistrationRequest(const Message& message) const = 0;
    virtual std::unique_ptr<DeviceRegistrationResponse> makeRegistrationResponse(const Message& message) const = 0;

    virtual bool isRegistrationRequest(const Message& message) const = 0;
    virtual bool isRegistrationResponse(const Message& message) const = 0;

    virtual bool isReregistrationRequest(const Message& message) const = 0;
    virtual bool isReregistrationResponse(const Message& message) const = 0;

    virtual bool isDeviceDeletionRequest(const Message& message) const = 0;
    virtual bool isDeviceDeletionResponse(const Message& message) const = 0;

    virtual std::string getResponseChannel(const Message& message, const std::string& gatewayKey,
                                           const std::string& deviceKey) const = 0;
};
}    // namespace wolkabout

#endif
