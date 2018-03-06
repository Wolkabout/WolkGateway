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

#include "InboundDeviceMessageHandler.h"
#include "InboundPlatformMessageHandler.h"
#include "model/Device.h"
#include "model/DeviceManifest.h"
#include "model/Message.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"
#include <functional>
#include <memory>
#include <string>

namespace wolkabout
{
class ChannelProtocolResolver : public PlatformMessageListener, public DeviceMessageListener
{
public:
    virtual ~ChannelProtocolResolver() = default;
    ChannelProtocolResolver(DeviceRepository& deviceRepository,
                            std::function<void(const std::string&, std::shared_ptr<Message>)> platformMessageHandler,
                            std::function<void(const std::string&, std::shared_ptr<Message>)> deviceMessageHandler);

protected:
    DeviceRepository& m_deviceRepository;

    std::function<void(const std::string&, std::shared_ptr<Message>)> m_platformMessageHandler;
    std::function<void(const std::string&, std::shared_ptr<Message>)> m_deviceMessageHandler;
};

template <class P> class ChannelProtocolResolverImpl : public ChannelProtocolResolver
{
public:
    ChannelProtocolResolverImpl(
      DeviceRepository& deviceRepository,
      std::function<void(const std::string&, std::shared_ptr<Message>)> platformMessageHandler,
      std::function<void(const std::string&, std::shared_ptr<Message>)> deviceMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;
};

template <class P>
ChannelProtocolResolverImpl<P>::ChannelProtocolResolverImpl(
  DeviceRepository& deviceRepository,
  std::function<void(const std::string&, std::shared_ptr<Message>)> platformMessageHandler,
  std::function<void(const std::string&, std::shared_ptr<Message>)> deviceMessageHandler)
: ChannelProtocolResolver(deviceRepository, platformMessageHandler, deviceMessageHandler)
{
}

template <class P> void ChannelProtocolResolverImpl<P>::platformMessageReceived(std::shared_ptr<Message> message)
{
    const std::string deviceKey = P::deviceKeyFromTopic(message->getChannel());
    std::shared_ptr<Device> device = m_deviceRepository.findByDeviceKey(deviceKey);
    if (!device)
    {
        LOG(DEBUG) << "Protocol Resolver: Device not found for " << message->getChannel();
        return;
    }

    const std::string protocol = device->getManifest().getProtocol();
    m_platformMessageHandler(protocol, message);
}

template <class P> void ChannelProtocolResolverImpl<P>::deviceMessageReceived(std::shared_ptr<Message> message)
{
    const std::string deviceKey = P::deviceKeyFromTopic(message->getChannel());
    std::shared_ptr<Device> device = m_deviceRepository.findByDeviceKey(deviceKey);
    if (!device)
    {
        LOG(DEBUG) << "Protocol Resolver: Device not found for " << message->getChannel();
        return;
    }

    const std::string protocol = device->getManifest().getProtocol();
    m_deviceMessageHandler(protocol, message);
}
}    // namespace wolkabout

#endif    // CHANNELPROTOCOLRESOLVER_H
