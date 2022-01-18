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

#include "StatusMessageRouter.h"

#include "core/model/Message.h"
#include "core/utilities/Logger.h"
#include "protocol/GatewayStatusProtocol.h"

namespace wolkabout
{
namespace gateway
{
StatusMessageRouter::StatusMessageRouter(GatewayStatusProtocol& gatewayProtocol,
                                         DeviceMessageListener* deviceStatusMessageHandler,
                                         DeviceMessageListener* lastWillMessageHandler)
: m_gatewayProtocol{gatewayProtocol}
, m_deviceStatusMessageHandler{deviceStatusMessageHandler}
, m_lastWillMessageHandler{lastWillMessageHandler}
{
}

void StatusMessageRouter::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << "Routing device status protocol message: " << message->getChannel();

    if (m_gatewayProtocol.isStatusResponseMessage(*message) && m_deviceStatusMessageHandler)
    {
        m_deviceStatusMessageHandler->deviceMessageReceived(message);
    }
    else if (m_gatewayProtocol.isStatusUpdateMessage(*message) && m_deviceStatusMessageHandler)
    {
        m_deviceStatusMessageHandler->deviceMessageReceived(message);
    }
    else if (m_gatewayProtocol.isLastWillMessage(*message) && m_lastWillMessageHandler)
    {
        m_lastWillMessageHandler->deviceMessageReceived(message);
    }
    else
    {
        LOG(WARN) << "Failed to route device status protocol message: " << message->getChannel();
    }
}

const GatewayProtocol& StatusMessageRouter::getGatewayProtocol() const
{
    return m_gatewayProtocol;
}
}    // namespace gateway
}    // namespace wolkabout
