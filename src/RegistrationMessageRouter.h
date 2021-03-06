/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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

#ifndef REGISTRATIONMESSAGEROUTER_H
#define REGISTRATIONMESSAGEROUTER_H

#include "GatewayInboundDeviceMessageHandler.h"
#include "GatewayInboundPlatformMessageHandler.h"

#include <memory>

namespace wolkabout
{
class GatewaySubdeviceRegistrationProtocol;
class Message;
class RegistrationProtocol;

class RegistrationMessageRouter : public PlatformMessageListener, public DeviceMessageListener
{
public:
    RegistrationMessageRouter(RegistrationProtocol& protocol, GatewaySubdeviceRegistrationProtocol& gatewayProtocol,
                              PlatformMessageListener* PlatformGatewayUpdateResponseMessageHandler,
                              DeviceMessageListener* DeviceSubdeviceRegistrationRequestMessageHandler,
                              DeviceMessageListener* DeviceSubdeviceUpdateRequestMessageHandler,
                              PlatformMessageListener* PlatformSubdeviceRegistrationResponseMessageHandler,
                              PlatformMessageListener* PlatformSubdeviceDeletionResponseMessageHandler,
                              PlatformMessageListener* PlatformSubdeviceUpdateResponseMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;
    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    const GatewayProtocol& getGatewayProtocol() const override;
    const Protocol& getProtocol() const override;

private:
    RegistrationProtocol& m_protocol;
    GatewaySubdeviceRegistrationProtocol& m_gatewayProtocol;

    PlatformMessageListener* m_platformGatewayUpdateResponseMessageHandler;
    DeviceMessageListener* m_deviceSubdeviceRegistrationRequestMessageHandler;
    DeviceMessageListener* m_deviceSubdeviceUpdateRequestMessageHandler;
    PlatformMessageListener* m_platformSubdeviceRegistrationResponseMessageHandler;
    PlatformMessageListener* m_platformSubdeviceDeletionResponseMessageHandler;
    PlatformMessageListener* m_platformSubdeviceUpdateResponseMessageHandler;
};
}    // namespace wolkabout

#endif    // REGISTRATIONMESSAGEROUTER_H
