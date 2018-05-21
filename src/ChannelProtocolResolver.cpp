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

#include "ChannelProtocolResolver.h"
#include "model/DetailedDevice.h"
#include "model/Message.h"
#include "protocol/GatewayDataProtocol.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"

namespace wolkabout
{
ChannelProtocolResolver::ChannelProtocolResolver(
  GatewayDataProtocol& protocol, DeviceRepository& deviceRepository,
  std::function<void(const std::string&, std::shared_ptr<Message>)> platformMessageHandler,
  std::function<void(const std::string&, std::shared_ptr<Message>)> deviceMessageHandler)
: m_protocol{protocol}
, m_deviceRepository{deviceRepository}
, m_platformMessageHandler{platformMessageHandler}
, m_deviceMessageHandler{deviceMessageHandler}
{
}

void ChannelProtocolResolver::platformMessageReceived(std::shared_ptr<Message> message)
{
    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(message->getChannel());
    std::shared_ptr<DetailedDevice> device = m_deviceRepository.findByDeviceKey(deviceKey);
    if (!device)
    {
        LOG(DEBUG) << "Protocol Resolver: Device not found for " << message->getChannel();
        return;
    }

    const std::string protocol = device->getManifest().getProtocol();
    m_platformMessageHandler(protocol, message);
}

void ChannelProtocolResolver::deviceMessageReceived(std::shared_ptr<Message> message)
{
    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(message->getChannel());
    std::shared_ptr<DetailedDevice> device = m_deviceRepository.findByDeviceKey(deviceKey);
    if (!device)
    {
        LOG(DEBUG) << "Protocol Resolver: Device not found for " << message->getChannel();
        return;
    }

    const std::string protocol = device->getManifest().getProtocol();
    m_deviceMessageHandler(protocol, message);
}

const GatewayProtocol& ChannelProtocolResolver::getProtocol() const
{
    return m_protocol;
}
}    // namespace wolkabout
