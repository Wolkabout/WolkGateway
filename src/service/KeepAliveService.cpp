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

#include "service/KeepAliveService.h"
#include "OutboundMessageHandler.h"
#include "connectivity/ConnectivityService.h"
#include "connectivity/json/StatusProtocol.h"

namespace wolkabout
{
KeepAliveService::KeepAliveService(std::string gatewayKey, OutboundMessageHandler& outboundMessageHandler,
                                   std::chrono::seconds keepAliveInterval)
: m_gatewayKey{std::move(gatewayKey)}
, m_outboundMessageHandler{outboundMessageHandler}
, m_keepAliveInterval{std::move(keepAliveInterval)}
{
}

void KeepAliveService::platformMessageReceived(std::shared_ptr<Message> message) {}

void KeepAliveService::connected()
{
    auto ping = [=] {
        auto message = StatusProtocol::makeFromPingRequest(m_gatewayKey);

        if (message)
        {
            m_outboundMessageHandler.addMessage(message);
        }
    };

    // send as soon as connected
    ping();

    m_timer.run(std::chrono::duration_cast<std::chrono::milliseconds>(m_keepAliveInterval), ping);
}

void KeepAliveService::disconnected()
{
    m_timer.stop();
}
}
