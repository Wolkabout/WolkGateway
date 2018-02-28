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

#ifndef CHANNELPROTOCOLRESOLVER_H
#define CHANNELPROTOCOLRESOLVER_H

#include "DeviceManager.h"
#include "InboundDeviceMessageHandler.h"
#include "InboundPlatformMessageHandler.h"
#include "model/Message.h"
#include <functional>
#include <memory>
#include <string>

namespace wolkabout
{
class DeviceManager;

class ChannelProtocolResolver : public PlatformMessageListener, public DeviceMessageListener
{
public:
    virtual ~ChannelProtocolResolver() = default;
    ChannelProtocolResolver(DeviceManager& deviceManager,
                            std::function<void(const std::string&, std::shared_ptr<Message>)> platformMessageHandler,
                            std::function<void(const std::string&, std::shared_ptr<Message>)> deviceMessageHandler);

protected:
    DeviceManager& m_deviceManger;

    std::function<void(const std::string&, std::shared_ptr<Message>)> m_platformMessageHandler;
    std::function<void(const std::string&, std::shared_ptr<Message>)> m_deviceMessageHandler;
};

template <class P> class ChannelProtocolResolverImpl : public ChannelProtocolResolver
{
public:
    ChannelProtocolResolverImpl(
      DeviceManager& deviceManager,
      std::function<void(const std::string&, std::shared_ptr<Message>)> platformMessageHandler,
      std::function<void(const std::string&, std::shared_ptr<Message>)> deviceMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;
};

template <class P>
ChannelProtocolResolverImpl<P>::ChannelProtocolResolverImpl(
  DeviceManager& deviceManager,
  std::function<void(const std::string&, std::shared_ptr<Message>)> platformMessageHandler,
  std::function<void(const std::string&, std::shared_ptr<Message>)> deviceMessageHandler)
: ChannelProtocolResolver(deviceManager, platformMessageHandler, deviceMessageHandler)
{
}

template <class P> void ChannelProtocolResolverImpl<P>::platformMessageReceived(std::shared_ptr<Message> message)
{
    const std::string deviceKey = P::getInstance().deviceKeyFromTopic(message->getTopic());

    const std::string protocol = m_deviceManger.getProtocolForDevice(deviceKey);

    m_platformMessageHandler(protocol, message);
}

template <class P> void ChannelProtocolResolverImpl<P>::deviceMessageReceived(std::shared_ptr<Message> message)
{
    const std::string deviceKey = P::getInstance().deviceKeyFromTopic(message->getTopic());

    const std::string protocol = m_deviceManger.getProtocolForDevice(deviceKey);

    m_deviceMessageHandler(protocol, message);
}
}

#endif    // CHANNELPROTOCOLRESOLVER_H
