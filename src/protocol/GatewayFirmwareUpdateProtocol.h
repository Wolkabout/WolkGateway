/*
 * Copyright 2019 WolkAbout Technology s.r.o.
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
#include <string>

namespace wolkabout
{
class FirmwareUpdateAbort;
class FirmwareUpdateInstall;
class FirmwareUpdateStatus;
class FirmwareVersion;
class Message;

class GatewayFirmwareUpdateProtocol : public GatewayProtocol
{
public:
    virtual std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                                 const FirmwareUpdateAbort& command) const = 0;

    virtual std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                                 const FirmwareUpdateInstall& command) const = 0;

    virtual std::unique_ptr<FirmwareVersion> makeFirmwareVersion(const Message& message) const = 0;

    virtual std::unique_ptr<FirmwareUpdateStatus> makeFirmwareUpdateStatus(const Message& message) const = 0;
};
}    // namespace wolkabout

#endif    // GATEWAYFIRMWAREUPDATEPROTOCOL_H
