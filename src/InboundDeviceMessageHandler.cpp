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

#include "InboundDeviceMessageHandler.h"
#include "connectivity/Channels.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include <algorithm>

namespace wolkabout
{
InboundDeviceMessageHandler::InboundDeviceMessageHandler() : m_commandBuffer{new CommandBuffer()} {}

void InboundDeviceMessageHandler::messageReceived(const std::string& channel, const std::string& payload)
{
    LOG(TRACE) << "InboundDeviceMessageHandler: Received message on channel: '" << channel << "'. Payload: '" << payload << "'";

    std::lock_guard<std::mutex> lg{m_lock};

    auto it = std::find_if(m_topicHandlers.begin(), m_topicHandlers.end(),
                           [&](const std::pair<std::string, std::weak_ptr<DeviceMessageListener>>& kvp) {
                               return StringUtils::mqttTopicMatch(kvp.first, channel);
                           });

    if (it != m_topicHandlers.end())
    {
        auto topicHandler = it->second;
        addToCommandBuffer([=] {
            if (auto handler = topicHandler.lock())
            {
                handler->deviceMessageReceived(std::make_shared<Message>(payload, channel));
            }
        });
    }
    else
    {
        LOG(ERROR) << "InboundDeviceMessageHandler: Handler for device topic not found: " << channel;
    }
}

const std::vector<std::string>& InboundDeviceMessageHandler::getTopics() const
{
    std::lock_guard<std::mutex> lg{m_lock};
    return m_subscriptionList;
}

void InboundDeviceMessageHandler::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(command));
}
}    // namespace wolkabout
