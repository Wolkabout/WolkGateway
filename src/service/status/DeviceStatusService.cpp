/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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

#include "DeviceStatusService.h"

#include "ConnectionStatusListener.h"
#include "OutboundMessageHandler.h"
#include "core/model/DeviceStatus.h"
#include "core/model/Message.h"
#include "core/protocol/StatusProtocol.h"
#include "core/utilities/Logger.h"
#include "protocol/GatewayStatusProtocol.h"
#include "repository/DeviceRepository.h"

namespace wolkabout
{
DeviceStatusService::DeviceStatusService(std::string gatewayKey, StatusProtocol& protocol,
                                         OutboundMessageHandler& outboundPlatformMessageHandler)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
{
}

void DeviceStatusService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = message->getChannel();

    if (m_protocol.isStatusRequestMessage(*message))
    {
        const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(topic);

        if (deviceKey.empty())
        {
            return;
        }

        requestDeviceStatus(deviceKey);
    }
    else if (m_protocol.isStatusConfirmMessage(*message))
    {
    }
    else
    {
        LOG(WARN) << "Message channel not parsed: " << topic;
    }
}

const Protocol& DeviceStatusService::getProtocol() const
{
    return m_protocol;
}

void DeviceStatusService::sendStatusUpdateForDevice(const std::string& deviceKey, DeviceStatus::Status status)
{
    std::shared_ptr<Message> statusMessage =
      m_protocol.makeStatusUpdateMessage(m_gatewayKey, DeviceStatus{deviceKey, status});

    if (!statusMessage)
    {
        LOG(WARN) << "Failed to create status update message for device: " << deviceKey;
        return;
    }

    m_outboundPlatformMessageHandler.addMessage(statusMessage);
}

void DeviceStatusService::sendStatusResponseForDevice(const std::string& deviceKey, DeviceStatus::Status status)
{
    std::shared_ptr<Message> statusMessage =
      m_protocol.makeStatusResponseMessage(m_gatewayKey, DeviceStatus{deviceKey, status});

    if (!statusMessage)
    {
        LOG(WARN) << "Failed to create status response message for device: " << deviceKey;
        return;
    }

    m_outboundPlatformMessageHandler.addMessage(statusMessage);
}

}    // namespace wolkabout
