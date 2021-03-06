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

#include "RegistrationMessageRouter.h"

#include "core/model/Message.h"
#include "core/protocol/RegistrationProtocol.h"
#include "core/utilities/Logger.h"
#include "protocol/GatewaySubdeviceRegistrationProtocol.h"

namespace wolkabout
{
RegistrationMessageRouter::RegistrationMessageRouter(
  RegistrationProtocol& protocol, GatewaySubdeviceRegistrationProtocol& gatewayProtocol,
  PlatformMessageListener* PlatformGatewayUpdateResponseMessageHandler,
  DeviceMessageListener* DeviceSubdeviceRegistrationRequestMessageHandler,
  DeviceMessageListener* DeviceSubdeviceUpdateRequestMessageHandler,
  PlatformMessageListener* PlatformSubdeviceRegistrationResponseMessageHandler,
  PlatformMessageListener* PlatformSubdeviceDeletionResponseMessageHandler,
  PlatformMessageListener* PlatformSubdeviceUpdateResponseMessageHandler)
: m_protocol{protocol}
, m_gatewayProtocol{gatewayProtocol}
, m_platformGatewayUpdateResponseMessageHandler{PlatformGatewayUpdateResponseMessageHandler}
, m_deviceSubdeviceRegistrationRequestMessageHandler{DeviceSubdeviceRegistrationRequestMessageHandler}
, m_deviceSubdeviceUpdateRequestMessageHandler{DeviceSubdeviceUpdateRequestMessageHandler}
, m_platformSubdeviceRegistrationResponseMessageHandler{PlatformSubdeviceRegistrationResponseMessageHandler}
, m_platformSubdeviceDeletionResponseMessageHandler{PlatformSubdeviceDeletionResponseMessageHandler}
, m_platformSubdeviceUpdateResponseMessageHandler{PlatformSubdeviceUpdateResponseMessageHandler}
{
}

void RegistrationMessageRouter::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << "Routing platform registration protocol message: " << message->getChannel();

    if (m_protocol.isGatewayUpdateResponse(*message) && m_platformGatewayUpdateResponseMessageHandler)
    {
        m_platformGatewayUpdateResponseMessageHandler->platformMessageReceived(message);
    }
    else if (m_protocol.isSubdeviceDeletionResponse(*message) && m_platformSubdeviceDeletionResponseMessageHandler)
    {
        m_platformSubdeviceDeletionResponseMessageHandler->platformMessageReceived(message);
    }
    else if (m_protocol.isSubdeviceRegistrationResponse(*message) &&
             m_platformSubdeviceRegistrationResponseMessageHandler)
    {
        m_platformSubdeviceRegistrationResponseMessageHandler->platformMessageReceived(message);
    }
    else if (m_protocol.isSubdeviceUpdateResponse(*message) && m_platformSubdeviceUpdateResponseMessageHandler)
    {
        m_platformSubdeviceUpdateResponseMessageHandler->platformMessageReceived(message);
    }
    else
    {
        LOG(WARN) << "Failed to route platform registration protocol message: " << message->getChannel();
    }
}

void RegistrationMessageRouter::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << "Routing device registration protocol message: " << message->getChannel();

    if (m_gatewayProtocol.isSubdeviceRegistrationRequest(*message) &&
        m_deviceSubdeviceRegistrationRequestMessageHandler)
    {
        m_deviceSubdeviceRegistrationRequestMessageHandler->deviceMessageReceived(message);
    }
    else if (m_gatewayProtocol.isSubdeviceUpdateRequest(*message) && m_deviceSubdeviceUpdateRequestMessageHandler)
    {
        m_deviceSubdeviceUpdateRequestMessageHandler->deviceMessageReceived(message);
    }
    else
    {
        LOG(WARN) << "Failed to route device registration protocol message: " << message->getChannel();
    }
}

const GatewayProtocol& RegistrationMessageRouter::getGatewayProtocol() const
{
    return m_gatewayProtocol;
}

const Protocol& RegistrationMessageRouter::getProtocol() const
{
    return m_protocol;
}
}    // namespace wolkabout
