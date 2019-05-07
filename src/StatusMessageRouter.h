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
#ifndef STATUSMESSAGEROUTER_H
#define STATUSMESSAGEROUTER_H

#include "GatewayInboundDeviceMessageHandler.h"
#include "GatewayInboundPlatformMessageHandler.h"
#include <memory>

namespace wolkabout
{
class GatewayStatusProtocol;
class Message;
class StatusProtocol;

class StatusMessageRouter : public PlatformMessageListener, public DeviceMessageListener
{
public:
    StatusMessageRouter(StatusProtocol& protocol, GatewayStatusProtocol& gatewayProtocol,
                        PlatformMessageListener* platformStatusMessageHandler,
                        DeviceMessageListener* deviceStatusMessageHandler,
                        DeviceMessageListener* lastWillMessageHandler,
                        PlatformMessageListener* platformKeepAliveMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;
    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    const Protocol& getProtocol() const override;
    const GatewayProtocol& getGatewayProtocol() const override;

private:
    StatusProtocol& m_protocol;
    GatewayStatusProtocol& m_gatewayProtocol;

    PlatformMessageListener* m_platformStatusMessageHandler;
    DeviceMessageListener* m_deviceStatusMessageHandler;
    DeviceMessageListener* m_lastWillMessageHandler;
    PlatformMessageListener* m_platformKeepAliveMessageHandler;
};
}    // namespace wolkabout

#endif    // STATUSMESSAGEROUTER_H
