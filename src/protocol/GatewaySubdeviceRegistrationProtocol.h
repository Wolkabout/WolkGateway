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

#ifndef GATEWAYSUBDEVICEREGISTRATIONPROTOCOL_H
#define GATEWAYSUBDEVICEREGISTRATIONPROTOCOL_H

#include "protocol/GatewayProtocol.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class SubdeviceRegistrationRequest;
class SubdeviceRegistrationResponse;
class GatewayUpdateRequest;
class GatewayUpdateResponse;
class GatewaySubdeviceRegistrationProtocol : public GatewayProtocol
{
public:
    GatewayProtocol::Type getType() const override final { return GatewayProtocol::Type::REGISTRATION; }

    virtual std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                                 const SubdeviceRegistrationRequest& request) const = 0;

    virtual std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                                 const SubdeviceRegistrationResponse& request) const = 0;

    virtual std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                                 const GatewayUpdateRequest& request) const = 0;

    virtual std::unique_ptr<Message> makeSubdeviceDeletionRequestMessage(const std::string& gatewayKey,
                                                                         const std::string& deviceKey) const = 0;

    virtual std::unique_ptr<SubdeviceRegistrationRequest> makeSubdeviceRegistrationRequest(
      const Message& message) const = 0;
    virtual std::unique_ptr<SubdeviceRegistrationResponse> makeSubdeviceRegistrationResponse(
      const Message& message) const = 0;

    virtual std::unique_ptr<GatewayUpdateResponse> makeGatewayUpdateResponse(const Message& message) const = 0;

    virtual bool isSubdeviceRegistrationRequest(const Message& message) const = 0;
    virtual bool isSubdeviceRegistrationResponse(const Message& message) const = 0;

    virtual bool isGatewayUpdateResponse(const Message& message) const = 0;
    virtual bool isGatewayUpdateRequest(const Message& message) const = 0;

    virtual bool isSubdeviceDeletionRequest(const Message& message) const = 0;
    virtual bool isSubdeviceDeletionResponse(const Message& message) const = 0;

    virtual std::string getResponseChannel(const Message& message, const std::string& gatewayKey,
                                           const std::string& deviceKey) const = 0;
};
}    // namespace wolkabout

#endif
