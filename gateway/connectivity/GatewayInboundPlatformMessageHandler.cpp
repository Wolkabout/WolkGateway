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

#include "gateway/connectivity/GatewayInboundPlatformMessageHandler.h"

#include "core/protocol/GatewaySubdeviceProtocol.h"
#include "core/utilities/Logger.h"

namespace wolkabout
{
namespace gateway
{
GatewayInboundPlatformMessageHandler::GatewayInboundPlatformMessageHandler(
  std::string gatewayKey, wolkabout::GatewaySubdeviceProtocol& protocol)
: m_gatewayKey(std::move(gatewayKey)), m_protocol(protocol)
{
}

void GatewayInboundPlatformMessageHandler::messageReceived(const std::string& topic, const std::string& message)
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::mutex> lock{m_mutex};

    // Look if we can figure out the type of the message
    LOG(TRACE) << TAG << "Topic: '" << topic << "' | Payload: '" << message << "'.";
    auto sharedMessage = std::make_shared<Message>(message, topic);
    auto messageType = m_protocol.getMessageType(*sharedMessage);
    if (messageType == MessageType::UNKNOWN)
    {
        LOG(ERROR) << TAG << "Received a message but failed to recognize the type.";
        return;
    }

    // Find a handler for the type
    auto handlerIt = m_listenersPerType.find(messageType);
    if (handlerIt == m_listenersPerType.cend())
    {
        LOG(DEBUG) << TAG << "Received a message but no handlers listen to the type.";
        return;
    }
    auto handler = handlerIt->second.lock();
    if (handler == nullptr)
    {
        LOG(DEBUG) << TAG << "Received a message but the handler for it has expired.";
        return;
    }

    // Parse the message
    auto parsedMessage = m_protocol.parseIncomingSubdeviceMessage(sharedMessage);
    if (parsedMessage.empty())
    {
        LOG(ERROR) << TAG << "Received a message but failed to parse any subdevice messages from it.";
        return;
    }

    // Take all the messages and route them to the handler
    m_commandBuffer.pushCommand(
      std::make_shared<std::function<void()>>([parsedMessage, handler] { handler->receiveMessages(parsedMessage); }));
}

std::vector<std::string> GatewayInboundPlatformMessageHandler::getChannels() const
{
    return m_protocol.getInboundChannelsForDevice(m_gatewayKey);
}

void GatewayInboundPlatformMessageHandler::addListener(const std::string& name,
                                                       const std::shared_ptr<GatewayMessageListener>& listener)
{
    LOG(TRACE) << METHOD_INFO;

    // Check if the message types are not empty
    const auto& messageTypes = listener->getMessageTypes();
    if (messageTypes.empty())
    {
        LOG(WARN) << TAG << "Attempted to add listener '" << name << "' but listener listens to no MessageType values.";
        return;
    }

    // Add it to the registry
    m_listeners.emplace(name, listener);
    for (const auto& messageType : messageTypes)
    {
        m_listenersPerType[messageType] = listener;
        LOG(DEBUG) << TAG << "Added listener '" << name << "' for type '" << toString(messageType) << "'.";
    }
}
}    // namespace gateway
}    // namespace wolkabout
