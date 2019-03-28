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

#ifndef KEEPALIVESERVICE_H
#define KEEPALIVESERVICE_H

#include "ConnectionStatusListener.h"
#include "InboundPlatformMessageHandler.h"
#include "utilities/Timer.h"

namespace wolkabout
{
class OutboundMessageHandler;
class StatusProtocol;

class KeepAliveService : public PlatformMessageListener, public ConnectionStatusListener
{
public:
    KeepAliveService(std::string gatewayKey, StatusProtocol& protocol, OutboundMessageHandler& outboundMessageHandler,
                     std::chrono::seconds keepAliveInterval);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    const Protocol& getProtocol() const override;

    void connected() override;
    void disconnected() override;

    void sendPingMessage() const;

private:
    const std::string m_gatewayKey;

    StatusProtocol& m_protocol;
    OutboundMessageHandler& m_outboundMessageHandler;

    const std::chrono::seconds m_keepAliveInterval;

    Timer m_timer;
};
}    // namespace wolkabout

#endif    // KEEPALIVESERVICE_H
