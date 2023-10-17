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

#include "gateway/service/external_data/ExternalDataService.h"

#include "core/connectivity/OutboundMessageHandler.h"
#include "core/model/Message.h"
#include "core/protocol/DataProtocol.h"
#include "core/protocol/GatewaySubdeviceProtocol.h"
#include "core/utility/Logger.h"

#include <algorithm>

using namespace wolkabout::legacy;

namespace wolkabout::gateway
{
ExternalDataService::ExternalDataService(std::string gatewayKey, GatewaySubdeviceProtocol& gatewaySubdeviceProtocol,
                                         DataProtocol& dataProtocol, OutboundMessageHandler& outboundMessageHandler,
                                         DataProvider& dataProvider)
: m_gatewayKey{std::move(gatewayKey)}
, m_gatewaySubdeviceProtocol{gatewaySubdeviceProtocol}
, m_dataProtocol{dataProtocol}
, m_outboundMessageHandler{outboundMessageHandler}
, m_dataProvider{dataProvider}
{
}

std::vector<MessageType> ExternalDataService::getMessageTypes() const
{
    return {MessageType::FEED_VALUES, MessageType::PARAMETER_SYNC};
}

void ExternalDataService::receiveMessages(const std::vector<GatewaySubdeviceMessage>& messages)
{
    LOG(TRACE) << METHOD_INFO;
    LOG(DEBUG) << TAG << "Received " << messages.size() << " messages.";

    // Check that the vector is not empty
    if (messages.empty())
    {
        LOG(WARN) << TAG << "Received a vector containing no subdevice messages.";
        return;
    }

    // Parse each message and call the data handler for them
    for (const auto& message : messages)
    {
        // Obtain the message type and the device key
        const auto& content = message.getMessage();
        auto messageType = m_gatewaySubdeviceProtocol.getMessageType(message.getMessage());
        auto deviceKey = m_gatewaySubdeviceProtocol.getDeviceKey(message.getMessage());
        auto sharedMessage = std::make_shared<Message>(content.getContent(), content.getChannel());

        // Parse it into an appropriate type and pass to the handler
        switch (messageType)
        {
        case MessageType::FEED_VALUES:
        {
            const auto feedValuesMessage =
              std::shared_ptr<FeedValuesMessage>{m_dataProtocol.parseFeedValues(sharedMessage)};
            if (feedValuesMessage == nullptr)
            {
                LOG(ERROR) << TAG << "Received 'FeedValues' message but failed to parse it.";
                return;
            }
            m_commandBuffer.pushCommand(std::make_shared<std::function<void()>>(
              [this, deviceKey, feedValuesMessage]
              { m_dataProvider.onReadingData(deviceKey, feedValuesMessage->getReadings()); }));
            return;
        }
        case MessageType::PARAMETER_SYNC:
        {
            const auto parametersMessage =
              std::shared_ptr<ParametersUpdateMessage>{m_dataProtocol.parseParameters(sharedMessage)};
            if (parametersMessage == nullptr)
            {
                LOG(ERROR) << TAG << "Received 'Parameters' message but failed to parse it.";
                return;
            }
            m_commandBuffer.pushCommand(std::make_shared<std::function<void()>>(
              [this, deviceKey, parametersMessage]
              { m_dataProvider.onParameterData(deviceKey, parametersMessage->getParameters()); }));
            return;
        }
        default:
            LOG(WARN) << TAG << "Received a message of type that the service can not handle.";
        }
    }
}

void ExternalDataService::addReading(const std::string& deviceKey, const Reading& reading)
{
    LOG(TRACE) << METHOD_INFO;
    auto message = m_dataProtocol.makeOutboundMessage(deviceKey, FeedValuesMessage{{reading}});
    if (message == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to parse an outgoing `FeedValues` message.";
        return;
    }
    packMessageWithGatewayAndSend(*message);
}

void ExternalDataService::addReadings(const std::string& deviceKey, const std::vector<Reading>& readings)
{
    LOG(TRACE) << METHOD_INFO;
    auto message = m_dataProtocol.makeOutboundMessage(deviceKey, FeedValuesMessage{readings});
    if (message == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to parse an outgoing `FeedValues` message.";
        return;
    }
    packMessageWithGatewayAndSend(*message);
}

void ExternalDataService::pullFeedValues(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;
    auto message = m_dataProtocol.makeOutboundMessage(deviceKey, PullFeedValuesMessage{});
    if (message == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to parse an outgoing `PullFeedValues` message.";
        return;
    }
    packMessageWithGatewayAndSend(*message);
}

void ExternalDataService::pullParameters(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;
    auto message = m_dataProtocol.makeOutboundMessage(deviceKey, ParametersPullMessage{});
    if (message == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to parse an outgoing `ParametersPull` message.";
        return;
    }
    packMessageWithGatewayAndSend(*message);
}

void ExternalDataService::registerFeed(const std::string& deviceKey, const Feed& feed)
{
    LOG(TRACE) << METHOD_INFO;
    auto message = m_dataProtocol.makeOutboundMessage(deviceKey, FeedRegistrationMessage{{feed}});
    if (message == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to parse an outgoing `FeedRegistration` message.";
        return;
    }
    packMessageWithGatewayAndSend(*message);
}

void ExternalDataService::registerFeeds(const std::string& deviceKey, const std::vector<Feed>& feeds)
{
    LOG(TRACE) << METHOD_INFO;
    auto message = m_dataProtocol.makeOutboundMessage(deviceKey, FeedRegistrationMessage{feeds});
    if (message == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to parse an outgoing `FeedRegistration` message.";
        return;
    }
    packMessageWithGatewayAndSend(*message);
}

void ExternalDataService::removeFeed(const std::string& deviceKey, const std::string& reference)
{
    LOG(TRACE) << METHOD_INFO;
    auto message = m_dataProtocol.makeOutboundMessage(deviceKey, FeedRemovalMessage{{reference}});
    if (message == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to parse an outgoing `FeedRemoval` message.";
        return;
    }
    packMessageWithGatewayAndSend(*message);
}

void ExternalDataService::removeFeeds(const std::string& deviceKey, const std::vector<std::string>& references)
{
    LOG(TRACE) << METHOD_INFO;
    auto message = m_dataProtocol.makeOutboundMessage(deviceKey, FeedRemovalMessage{references});
    if (message == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to parse an outgoing `FeedRemoval` message.";
        return;
    }
    packMessageWithGatewayAndSend(*message);
}

void ExternalDataService::addAttribute(const std::string& deviceKey, Attribute attribute)
{
    LOG(TRACE) << METHOD_INFO;
    auto message = m_dataProtocol.makeOutboundMessage(deviceKey, AttributeRegistrationMessage{{attribute}});
    if (message == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to parse an outgoing `AttributeRegistration` message.";
        return;
    }
    packMessageWithGatewayAndSend(*message);
}

void ExternalDataService::updateParameter(const std::string& deviceKey, Parameter parameter)
{
    LOG(TRACE) << METHOD_INFO;
    auto message = m_dataProtocol.makeOutboundMessage(deviceKey, ParametersUpdateMessage{{parameter}});
    if (message == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to parse an outgoing `ParametersUpdate` message.";
        return;
    }
    packMessageWithGatewayAndSend(*message);
}

void ExternalDataService::packMessageWithGatewayAndSend(const Message& message)
{
    // Pack the message with the gateway protocol
    auto gatewayMessage = std::shared_ptr<Message>{
      m_gatewaySubdeviceProtocol.makeOutboundMessage(m_gatewayKey, GatewaySubdeviceMessage{message})};
    if (gatewayMessage == nullptr)
    {
        LOG(ERROR) << TAG << "Failed to pack the message in a gateway message.";
        return;
    }

    // Hand it to the outbound message handler
    m_outboundMessageHandler.addMessage(gatewayMessage);
}
}    // namespace wolkabout::gateway
