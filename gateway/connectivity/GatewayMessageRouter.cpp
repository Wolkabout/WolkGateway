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

#include "gateway/connectivity/GatewayMessageRouter.h"

#include "core/protocol/GatewaySubdeviceProtocol.h"
#include "core/utilities/Logger.h"

namespace wolkabout
{
namespace gateway
{
GatewayMessageRouter::GatewayMessageRouter(wolkabout::GatewaySubdeviceProtocol& protocol) : m_protocol(protocol) {}

void GatewayMessageRouter::messageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::mutex> lock{m_mutex};

    // Look if we can figure out the type of the message
    LOG(TRACE) << TAG << "Topic: '" << message->getChannel() << "' | Payload: '" << message->getContent() << "'.";
    auto messageType = m_protocol.getMessageType(*message);
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
        m_listenersPerType.erase(handlerIt);
        LOG(DEBUG) << TAG << "Received a message but the handler for it has expired. Deleting...";
        return;
    }

    // Parse the message
    auto parsedMessage = m_protocol.parseIncomingSubdeviceMessage(message);
    if (parsedMessage.empty())
    {
        LOG(ERROR) << TAG << "Received a message but failed to parse any subdevice messages from it.";
        return;
    }

    // Take all the messages and route them to the handler
    m_commandBuffer.pushCommand(
      std::make_shared<std::function<void()>>([parsedMessage, handler] { handler->receiveMessages(parsedMessage); }));
}

const Protocol& GatewayMessageRouter::getProtocol()
{
    return m_protocol;
}

void GatewayMessageRouter::addListener(const std::string& name, const std::shared_ptr<GatewayMessageListener>& listener)
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
