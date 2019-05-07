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
#include "model/Message.h"
#include "protocol/StatusProtocol.h"

namespace wolkabout
{
KeepAliveService::KeepAliveService(std::string gatewayKey, StatusProtocol& protocol,
                                   OutboundMessageHandler& outboundMessageHandler,
                                   std::chrono::seconds keepAliveInterval)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_outboundMessageHandler{outboundMessageHandler}
, m_keepAliveInterval{std::move(keepAliveInterval)}
{
}

void KeepAliveService::platformMessageReceived(std::shared_ptr<Message> message) {}

const Protocol& KeepAliveService::getProtocol() const
{
    return m_protocol;
}

void KeepAliveService::connected()
{
    // send as soon as connected
    sendPingMessage();

    m_timer.run(std::chrono::duration_cast<std::chrono::milliseconds>(m_keepAliveInterval), [=] { sendPingMessage(); });
}

void KeepAliveService::disconnected()
{
    m_timer.stop();
}

void KeepAliveService::sendPingMessage() const
{
    std::shared_ptr<Message> message = m_protocol.makeFromPingRequest(m_gatewayKey);

    if (message)
    {
        m_outboundMessageHandler.addMessage(message);
    }
}
}    // namespace wolkabout
