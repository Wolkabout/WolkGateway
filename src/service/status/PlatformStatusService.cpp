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

#include "PlatformStatusService.h"

#include "OutboundMessageHandler.h"
#include "core/model/Message.h"
#include "protocol/GatewayStatusProtocol.h"

namespace wolkabout
{

PlatformStatusService::PlatformStatusService(OutboundMessageHandler& outboundDeviceMessageHandler, GatewayStatusProtocol& protocol)
: m_outboundDeviceMessageHandler(outboundDeviceMessageHandler), m_protocol(protocol)
{
}

void PlatformStatusService::sendPlatformConnectionStatusMessage(const bool connected)
{
    std::shared_ptr<Message> message = m_protocol.makePlatformConnectionStatusMessage(connected);
    if(!message)
    {
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(message);
}

}   // namespace wolkabout
