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

#include "service/DeviceStatusService.h"
#include "OutboundMessageHandler.h"
#include "connectivity/json/StatusProtocol.h"
#include "model/DeviceStatusResponse.h"
#include "model/Message.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"

namespace wolkabout
{
DeviceStatusService::DeviceStatusService(std::string gatewayKey, DeviceRepository& deviceRepository,
                                         OutboundMessageHandler& outboundPlatformMessageHandler,
                                         OutboundMessageHandler& outboundDeviceMessageHandler)
: m_gatewayKey{std::move(gatewayKey)}
, m_deviceRepository{deviceRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
{
}

void DeviceStatusService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    const std::string topic = message->getChannel();

    if (StatusProtocol::getInstance().isPlatformToDeviceMessage(topic))
    {
        if (StatusProtocol::getInstance().isStatusRequestMessage(topic))
        {
            routePlatformMessage(message);
        }
        else
        {
            LOG(WARN) << "Incorrect status channel from platform: " << topic;
        }
    }
    else
    {
        LOG(WARN) << "Message channel not parsed: " << topic;
    }
}

void DeviceStatusService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    const std::string topic = message->getChannel();

    if (StatusProtocol::getInstance().isDeviceToPlatformMessage(topic))
    {
        if (StatusProtocol::getInstance().isStatusResponseMessage(topic))
        {
            routeDeviceMessage(message);
        }
        else if (StatusProtocol::getInstance().isLastWillMessage(topic))
        {
            const std::string deviceKey = StatusProtocol::getInstance().deviceKeyFromTopic(topic);
            if (deviceKey.empty())
            {
                // ping all devices
                auto deviceKeys = m_deviceRepository.findAllDeviceKeys();
                for (auto key : *deviceKeys)
                {
                    auto statusRequestMessage = StatusProtocol::getInstance().messageFromDeviceStatusRequest(key);
                    m_outboundDeviceMessageHandler.addMessage(statusRequestMessage);
                }
            }
            else
            {
                // offline status if last will contains device key
                auto statusMessage = StatusProtocol::getInstance().make(
                  m_gatewayKey, deviceKey,
                  std::make_shared<DeviceStatusResponse>(DeviceStatusResponse::Status::OFFLINE));

                if (!statusMessage)
                {
                    LOG(WARN) << "Failed to create device status message: " << topic << ", " << message->getContent();
                    return;
                }

                m_outboundPlatformMessageHandler.addMessage(statusMessage);
            }
        }
        else
        {
            LOG(WARN) << "Incorrect status channel from device: " << topic;
        }
    }
    else if (StatusProtocol::getInstance().isGatewayToPlatformMessage(topic))
    {
        if (StatusProtocol::getInstance().isStatusResponseMessage(topic))
        {
            // TODO handle gw module status
        }
        else if (StatusProtocol::getInstance().isLastWillMessage(topic))
        {
            // TODO handle gw module status
        }
        else
        {
            LOG(WARN) << "Incorrect status channel from gateway: " << topic;
        }
    }
    else
    {
        LOG(WARN) << "Message channel not parsed: " << topic;
    }
}

void DeviceStatusService::routeDeviceMessage(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    const std::string topic = StatusProtocol::getInstance().routeDeviceMessage(message->getChannel(), m_gatewayKey);
    if (topic.empty())
    {
        LOG(WARN) << "Failed to route device message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), topic)};

    m_outboundPlatformMessageHandler.addMessage(routedMessage);
}

void DeviceStatusService::routePlatformMessage(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    const std::string topic = StatusProtocol::getInstance().routePlatformMessage(message->getChannel(), m_gatewayKey);
    if (topic.empty())
    {
        LOG(WARN) << "Failed to route platform message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), topic)};

    m_outboundDeviceMessageHandler.addMessage(routedMessage);
}
}
