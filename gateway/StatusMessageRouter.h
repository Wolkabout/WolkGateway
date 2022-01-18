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

#include <memory>

namespace wolkabout
{
namespace gateway
{
class GatewayStatusProtocol;
class Message;

class StatusMessageRouter : public DeviceMessageListener
{
public:
    StatusMessageRouter(GatewayStatusProtocol& gatewayProtocol, DeviceMessageListener* deviceStatusMessageHandler,
                        DeviceMessageListener* lastWillMessageHandler);

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    const GatewayProtocol& getGatewayProtocol() const override;

private:
    GatewayStatusProtocol& m_gatewayProtocol;

    DeviceMessageListener* m_deviceStatusMessageHandler;
    DeviceMessageListener* m_lastWillMessageHandler;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // STATUSMESSAGEROUTER_H
