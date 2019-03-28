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
#include "model/Message.h"
#include "protocol/GatewayStatusProtocol.h"
#include "utilities/Logger.h"

namespace wolkabout
{
StatusMessageRouter::StatusMessageRouter(GatewayStatusProtocol& protocol,
                                         PlatformMessageListener* platformStatusMessageHandler,
                                         DeviceMessageListener* deviceStatusMessageHandler,
                                         DeviceMessageListener* lastWillMessageHandler,
                                         PlatformMessageListener* platformKeepAliveMessageHandler)
: m_protocol{protocol}
, m_platformStatusMessageHandler{platformStatusMessageHandler}
, m_deviceStatusMessageHandler{deviceStatusMessageHandler}
, m_lastWillMessageHandler{lastWillMessageHandler}
, m_platformKeepAliveMessageHandler{platformKeepAliveMessageHandler}
{
}

void StatusMessageRouter::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << "Routing platform status protocol message: " << message->getChannel();

    if (m_protocol.isStatusRequestMessage(*message) && m_platformStatusMessageHandler)
    {
        m_platformStatusMessageHandler->platformMessageReceived(message);
    }
    else if (m_protocol.isStatusConfirmMessage(*message) && m_platformStatusMessageHandler)
    {
        m_platformStatusMessageHandler->platformMessageReceived(message);
    }
    else if (m_protocol.isPongMessage(*message) && m_platformKeepAliveMessageHandler)
    {
        m_platformKeepAliveMessageHandler->platformMessageReceived(message);
    }
    else
    {
        LOG(WARN) << "Failed to route platform status protocol message: " << message->getChannel();
    }
}

void StatusMessageRouter::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << "Routing device status protocol message: " << message->getChannel();

    if (m_protocol.isStatusResponseMessage(*message) && m_deviceStatusMessageHandler)
    {
        m_deviceStatusMessageHandler->deviceMessageReceived(message);
    }
    else if (m_protocol.isStatusUpdateMessage(*message) && m_deviceStatusMessageHandler)
    {
        m_deviceStatusMessageHandler->deviceMessageReceived(message);
    }
    else if (m_protocol.isLastWillMessage(*message) && m_lastWillMessageHandler)
    {
        m_lastWillMessageHandler->deviceMessageReceived(message);
    }
    else
    {
        LOG(WARN) << "Failed to route device status protocol message: " << message->getChannel();
    }
}

const GatewayProtocol& StatusMessageRouter::getProtocol() const
{
    return m_protocol;
}
}    // namespace wolkabout
