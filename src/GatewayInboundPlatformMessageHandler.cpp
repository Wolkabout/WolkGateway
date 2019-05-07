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

#include "GatewayInboundPlatformMessageHandler.h"
#include "model/Message.h"
#include "protocol/Protocol.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"

#include <algorithm>

namespace wolkabout
{
GatewayInboundPlatformMessageHandler::GatewayInboundPlatformMessageHandler(const std::string& gatewayKey)
: m_commandBuffer{new CommandBuffer()}, m_gatewayKey{gatewayKey}
{
}

void GatewayInboundPlatformMessageHandler::messageReceived(const std::string& channel, const std::string& payload)
{
    // don't log binary payload
    if (!StringUtils::contains(channel, "binary"))
    {
        LOG(DEBUG) << "GatewayInboundPlatformMessageHandler: Message received on channel: '" << channel << "' : '"
                   << payload << "'";
    }
    else
    {
        LOG(DEBUG) << "GatewayInboundPlatformMessageHandler: Message received on channel: '" << channel << "'";
    }

    std::lock_guard<std::mutex> lg{m_lock};

    auto it = std::find_if(m_channelHandlers.begin(), m_channelHandlers.end(),
                           [&](const std::pair<std::string, std::weak_ptr<PlatformMessageListener>>& kvp) {
                               return StringUtils::mqttTopicMatch(kvp.first, channel);
                           });

    if (it != m_channelHandlers.end())
    {
        auto channelHandler = it->second;
        addToCommandBuffer([=] {
            if (auto handler = channelHandler.lock())
            {
                handler->platformMessageReceived(std::make_shared<Message>(payload, channel));
            }
        });
    }
    else
    {
        LOG(DEBUG) << "GatewayInboundPlatformMessageHandler: Handler for device channel not found: " << channel;
    }
}

std::vector<std::string> GatewayInboundPlatformMessageHandler::getChannels() const
{
    std::lock_guard<std::mutex> lg{m_lock};
    return m_subscriptionList;
}

void GatewayInboundPlatformMessageHandler::addListener(std::weak_ptr<PlatformMessageListener> listener)
{
    std::lock_guard<std::mutex> locker{m_lock};

    if (auto handler = listener.lock())
    {
        for (const auto& channel : handler->getProtocol().getInboundChannelsForDevice(m_gatewayKey))
        {
            LOG(DEBUG) << "Adding listener for channel: " << channel;
            m_channelHandlers[channel] = listener;
            m_subscriptionList.push_back(channel);
        }
    }
}

void GatewayInboundPlatformMessageHandler::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(command));
}
}    // namespace wolkabout
