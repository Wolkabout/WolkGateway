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

#ifndef WOLKABOUT_INTERNALDATASERVICE_H
#define WOLKABOUT_INTERNALDATASERVICE_H

#include "core/MessageListener.h"
#include "core/connectivity/OutboundMessageHandler.h"
#include "core/protocol/GatewaySubdeviceProtocol.h"
#include "gateway/GatewayMessageListener.h"

namespace wolkabout
{
namespace gateway
{
class InternalDataService : public GatewayMessageListener, public MessageListener
{
public:
    InternalDataService(std::string gatewayKey, OutboundMessageHandler& platformOutboundHandler,
                        OutboundMessageHandler& localOutboundHandler, GatewaySubdeviceProtocol& protocol);

    void messageReceived(std::shared_ptr<Message> message) override;
    const Protocol& getProtocol() override;

    void receiveMessages(const std::vector<GatewaySubdeviceMessage>& messages) override;

    std::vector<MessageType> getMessageTypes() const override;

private:
    // The gateway key
    std::string m_gatewayKey;

    // The connectivity objects
    OutboundMessageHandler& m_platformOutboundHandler;
    OutboundMessageHandler& m_localOutboundHandler;

    // The protocol
    GatewaySubdeviceProtocol& m_protocol;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // WOLKABOUT_INTERNALDATASERVICE_H
