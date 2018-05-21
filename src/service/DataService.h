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

#ifndef DATASERVICE_H
#define DATASERVICE_H

#include "ConnectionStatusListener.h"
#include "InboundDeviceMessageHandler.h"
#include "InboundPlatformMessageHandler.h"

#include <atomic>
#include <memory>
#include <string>

namespace wolkabout
{
class GatewayDataProtocol;
class OutboundMessageHandler;
class DeviceRepository;

class DataService : public DeviceMessageListener, public PlatformMessageListener, public ConnectionStatusListener
{
public:
    DataService(const std::string& gatewayKey, GatewayDataProtocol& protocol, DeviceRepository& deviceRepository,
                OutboundMessageHandler& outboundPlatformMessageHandler,
                OutboundMessageHandler& outboundDeviceMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    const GatewayProtocol& getProtocol() const override;

    void connected() override;

    void disconnected() override;

private:
    void routeDeviceToPlatformMessage(std::shared_ptr<Message> message);
    void routePlatformToDeviceMessage(std::shared_ptr<Message> message);

    void routeGatewayToPlatformMessage(std::shared_ptr<Message> message);
    void routePlatformToGatewayMessage(std::shared_ptr<Message> message);

    void handleGatewayOfflineMessage(std::shared_ptr<Message> message);

    const std::string m_gatewayKey;
    GatewayDataProtocol& m_protocol;

    DeviceRepository& m_deviceRepository;

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    OutboundMessageHandler& m_outboundDeviceMessageHandler;

    std::atomic_bool m_gatewayModuleConnected;
};

}    // namespace wolkabout

#endif
