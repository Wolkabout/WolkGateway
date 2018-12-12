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
#ifndef GATEWAYFIRMWAREUPDATEPROTOCOL_H
#define GATEWAYFIRMWAREUPDATEPROTOCOL_H

#include "protocol/GatewayProtocol.h"

#include <memory>

namespace wolkabout
{
class FirmwareUpdateCommand;
class FirmwareUpdateResponse;
class Message;

class GatewayFirmwareUpdateProtocol : public GatewayProtocol
{
public:
    GatewayProtocol::Type getType() const override final { return GatewayProtocol::Type::FIRMWARE_UPDATE; }

    virtual std::unique_ptr<Message> makeMessage(const std::string& gatewayKey, const std::string& deviceKey,
                                                 const FirmwareUpdateResponse& firmwareUpdateResponse) const = 0;

    virtual std::unique_ptr<Message> makeMessage(const std::string& deviceKey,
                                                 const FirmwareUpdateCommand& firmwareUpdateCommand) const = 0;

    virtual bool isFirmwareUpdateCommandMessage(const Message& message) const = 0;

    virtual bool isFirmwareUpdateResponseMessage(const Message& message) const = 0;

    virtual bool isFirmwareVersionMessage(const Message& message) const = 0;

    virtual std::string routeDeviceToPlatformMessage(const std::string& topic, const std::string& gatewayKey) const = 0;

    virtual std::unique_ptr<FirmwareUpdateCommand> makeFirmwareUpdateCommand(const Message& message) const = 0;

    virtual std::unique_ptr<FirmwareUpdateResponse> makeFirmwareUpdateResponse(const Message& message) const = 0;
};
}    // namespace wolkabout

#endif    // GATEWAYFIRMWAREUPDATEPROTOCOL_H
