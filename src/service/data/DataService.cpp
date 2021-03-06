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

#include "DataService.h"

#include "OutboundMessageHandler.h"
#include "core/InboundMessageHandler.h"
#include "core/model/Message.h"
#include "core/protocol/DataProtocol.h"
#include "core/utilities/Logger.h"
#include "protocol/GatewayDataProtocol.h"

#include <algorithm>
#include <cassert>

namespace wolkabout
{
DataService::DataService(const std::string& gatewayKey, DataProtocol& protocol, GatewayDataProtocol& gatewayProtocol,
                         OutboundMessageHandler& outboundPlatformMessageHandler, MessageListener* gatewayDevice)
: m_gatewayKey{gatewayKey}
, m_protocol{protocol}
, m_gatewayProtocol{gatewayProtocol}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_gatewayDevice{gatewayDevice}
{
}

void DataService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = message->getChannel();

    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(topic);

    if (deviceKey.empty())
    {
        LOG(WARN) << "DataService: Failed to extract device key from channel '" << topic << "'";
        return;
    }

    if (m_gatewayKey == deviceKey)
    {
        handleMessageForGateway(message);
    }
    else
    {
        // if message is for device remove gateway info from channel
        handleMessageForDevice(message);
    }
}

const Protocol& DataService::getProtocol() const
{
    return m_protocol;
}

void DataService::addMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    m_outboundPlatformMessageHandler.addMessage(message);
}

void DataService::setGatewayMessageListener(MessageListener* gatewayDevice)
{
    std::lock_guard<decltype(m_messageListenerMutex)> guard{m_messageListenerMutex};
    m_gatewayDevice = gatewayDevice;
}

void DataService::routeDeviceToPlatformMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = m_gatewayProtocol.routeDeviceToPlatformMessage(message->getChannel(), m_gatewayKey);
    if (channel.empty())
    {
        LOG(WARN) << "Failed to route device message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), channel)};

    addMessage(routedMessage);
}

void DataService::handleMessageForGateway(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    std::lock_guard<decltype(m_messageListenerMutex)> guard{m_messageListenerMutex};

    if (m_gatewayDevice)
    {
        m_gatewayDevice->messageReceived(message);
    }
}
}    // namespace wolkabout
