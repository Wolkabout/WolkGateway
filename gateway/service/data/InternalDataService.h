/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#ifndef WOLKABOUT_INTERNALDATASERVICE_H
#define WOLKABOUT_INTERNALDATASERVICE_H

#include "DataService.h"
#include "InboundDeviceMessageHandler.h"

namespace wolkabout
{
namespace gateway
{
class DeviceRepository;

class InternalDataService : public DataService, public DeviceMessageListener
{
public:
    InternalDataService(const std::string& gatewayKey, DataProtocol& protocol, GatewayProtocol& gatewayProtocol,
                        DeviceRepository* deviceRepository, OutboundMessageHandler& outboundPlatformMessageHandler,
                        OutboundMessageHandler& outboundDeviceMessageHandler, MessageListener* gatewayDevice = nullptr);

    const GatewayProtocol& getGatewayProtocol() const override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

private:
    void handleMessageForDevice(std::shared_ptr<Message> message) override;

    void routePlatformToDeviceMessage(std::shared_ptr<Message> message);

    DeviceRepository* m_deviceRepository;

    OutboundMessageHandler& m_outboundDeviceMessageHandler;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // WOLKABOUT_INTERNALDATASERVICE_H
