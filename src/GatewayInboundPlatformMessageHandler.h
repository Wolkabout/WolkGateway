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

#ifndef GATEWAYINBOUNDPLATFORMMESSAGEHANDLER_H
#define GATEWAYINBOUNDPLATFORMMESSAGEHANDLER_H

#include "InboundPlatformMessageHandler.h"
#include "core/utilities/CommandBuffer.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;

class GatewayInboundPlatformMessageHandler : public InboundPlatformMessageHandler
{
public:
    GatewayInboundPlatformMessageHandler(const std::string& gatewayKey);

    void messageReceived(const std::string& channel, const std::string& message) override;

    std::vector<std::string> getChannels() const override;

    void addListener(std::weak_ptr<PlatformMessageListener> listener) override;

private:
    void addToCommandBuffer(std::function<void()> command);

    std::unique_ptr<CommandBuffer> m_commandBuffer;
    const std::string m_gatewayKey;

    std::vector<std::string> m_subscriptionList;

    std::map<std::string, std::weak_ptr<PlatformMessageListener>> m_channelHandlers;

    mutable std::mutex m_lock;
};
}    // namespace wolkabout

#endif
