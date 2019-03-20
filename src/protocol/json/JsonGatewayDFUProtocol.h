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

#ifndef JSONGATEWAYDFUPROTOCOL_H
#define JSONGATEWAYDFUPROTOCOL_H

#include "protocol/GatewayFirmwareUpdateProtocol.h"

namespace wolkabout
{
class JsonGatewayDFUProtocol : public GatewayFirmwareUpdateProtocol
{
public:
    std::vector<std::string> getInboundChannels() const override;
    std::vector<std::string> getInboundChannelsForDevice(const std::string& deviceKey) const override;

    std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                         const FirmwareUpdateAbort& command) const override;

    std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                         const FirmwareUpdateInstall& command) const override;

    std::unique_ptr<FirmwareVersion> makeFirmwareVersion(const Message& message) const override;

    std::unique_ptr<FirmwareUpdateStatus> makeFirmwareUpdateStatus(const Message& message) const override;

private:
    FirmwareVersion firmwareVersionFromMessage(const Message& message) const;

    static const std::string FIRMWARE_UPDATE_STATUS_TOPIC_ROOT;
    static const std::string FIRMWARE_VERSION_TOPIC_ROOT;

    static const std::string FIRMWARE_UPDATE_INSTALL_TOPIC_ROOT;
    static const std::string FIRMWARE_UPDATE_ABORT_TOPIC_ROOT;
};
}    // namespace wolkabout

#endif    // JSONGATEWAYDFUPROTOCOL_H
