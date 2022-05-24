/**
 * Copyright 2021 Wolkabout s.r.o.
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

#ifndef WOLKGATEWAY_GATEWAYMESSAGEROUTER_H
#define WOLKGATEWAY_GATEWAYMESSAGEROUTER_H

#include "core/MessageListener.h"
#include "core/protocol/GatewaySubdeviceProtocol.h"
#include "core/utilities/CommandBuffer.h"
#include "gateway/GatewayMessageListener.h"

#include <functional>
#include <map>
#include <memory>

namespace wolkabout
{
namespace gateway
{
class GatewayMessageRouter : public MessageListener
{
public:
    explicit GatewayMessageRouter(GatewaySubdeviceProtocol& protocol);

    void messageReceived(std::shared_ptr<Message> message) override;

    const Protocol& getProtocol() override;

    virtual void addListener(const std::string& name, const std::shared_ptr<GatewayMessageListener>& listener);

private:
    // Logging tag
    const std::string TAG = "[GatewayInboundPlatformMessageHandler] -> ";

    // Protocol
    GatewaySubdeviceProtocol& m_protocol;

    // Message listeners
    std::mutex m_mutex;
    std::map<std::string, std::weak_ptr<GatewayMessageListener>> m_listeners;
    std::map<MessageType, std::weak_ptr<GatewayMessageListener>> m_listenersPerType;

    // Command buffer
    CommandBuffer m_commandBuffer;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // WOLKGATEWAY_GATEWAYMESSAGEROUTER_H
