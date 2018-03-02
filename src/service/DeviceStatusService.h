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

#ifndef DEVICESTATUSSERVICE_H
#define DEVICESTATUSSERVICE_H

#include "InboundDeviceMessageHandler.h"
#include "InboundPlatformMessageHandler.h"

namespace wolkabout
{
class DeviceRepository;
class ConnectionStatusListener;
class OutboundMessageHandler;

class DeviceStatusService : public DeviceMessageListener, public PlatformMessageListener
{
public:
    DeviceStatusService(std::string gatewayKey, DeviceRepository& deviceRepository,
                        OutboundMessageHandler& outboundPlatformMessageHandler,
                        OutboundMessageHandler& outboundDeviceMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    void setGatewayModuleConnectionStatusListener(std::weak_ptr<ConnectionStatusListener> listener);

private:
    void routeDeviceMessage(std::shared_ptr<Message> message);
    void routePlatformMessage(std::shared_ptr<Message> message);

    const std::string m_gatewayKey;
    DeviceRepository& m_deviceRepository;

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    OutboundMessageHandler& m_outboundDeviceMessageHandler;

    std::weak_ptr<ConnectionStatusListener> m_gatewayModuleConnectionStatusListener;
};
}

#endif    // DEVICESTATUSSERVICE_H
