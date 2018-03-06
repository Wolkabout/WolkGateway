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
#include "ConnectionStatusListener.h"
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

    if (StatusProtocol::isPlatformToDeviceMessage(topic))
    {
        if (StatusProtocol::isStatusRequestMessage(topic))
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

    if (StatusProtocol::isDeviceToPlatformMessage(topic))
    {
        if (StatusProtocol::isStatusResponseMessage(topic))
        {
            routeDeviceMessage(message);
        }
        else if (StatusProtocol::isLastWillMessage(topic))
        {
            const std::string deviceKey = StatusProtocol::deviceKeyFromTopic(topic);
            if (!deviceKey.empty())
            {
                LOG(INFO) << "Device Status Service: Device got disconnected: " << deviceKey;
                // offline status if last will topic contains device key
                auto statusMessage = StatusProtocol::messageFromDeviceStatusResponse(
                  m_gatewayKey, deviceKey, DeviceStatusResponse::Status::OFFLINE);

                if (!statusMessage)
                {
                    LOG(WARN) << "Failed to create device status message: " << topic << ", " << message->getContent();
                    return;
                }

                m_outboundPlatformMessageHandler.addMessage(statusMessage);
            }
            else    // check for list of key in payload
            {
                const auto deviceKeys = StatusProtocol::deviceKeysFromContent(topic);

                for (const auto& key : deviceKeys)
                {
                    LOG(INFO) << "Device Status Service: Device got disconnected: " << key;
                    auto statusMessage = StatusProtocol::messageFromDeviceStatusResponse(
                      m_gatewayKey, key, DeviceStatusResponse::Status::OFFLINE);

                    if (!statusMessage)
                    {
                        LOG(WARN) << "Failed to create device status message: " << topic << ", "
                                  << message->getContent();
                        return;
                    }

                    m_outboundPlatformMessageHandler.addMessage(statusMessage);
                }
            }
        }
        else
        {
            LOG(WARN) << "Device Status Service: Incorrect status channel from device: " << topic;
        }
    }
    else if (StatusProtocol::isGatewayToPlatformMessage(topic))
    {
        const std::string key = StatusProtocol::gatewayKeyFromTopic(topic);

        if (key != m_gatewayKey)
        {
            LOG(WARN) << "Device Status Service: Gateway key from channel does not match gateway: " << topic;
            return;
        }

        if (StatusProtocol::isStatusResponseMessage(topic))
        {
            auto deviceStatusResponse = StatusProtocol::makeDeviceStatusResponse(message);
            if (!deviceStatusResponse)
            {
                LOG(WARN) << "Device Status Service: Could not parse message: " << topic << ", "
                          << message->getContent();
                return;
            }

            switch (deviceStatusResponse->getStatus())
            {
            case DeviceStatusResponse::Status::CONNECTED:
            {
                if (auto handler = m_gatewayModuleConnectionStatusListener.lock())
                {
                    LOG(INFO) << "Device Status Service: Gateway module connected";
                    handler->connected();
                }
                break;
            }
            case DeviceStatusResponse::Status::SLEEP:
            case DeviceStatusResponse::Status::OFFLINE:
            {
                if (auto handler = m_gatewayModuleConnectionStatusListener.lock())
                {
                    LOG(INFO) << "Device Status Service: Gateway module got disconnected";
                    handler->disconnected();
                }
                break;
            }
            case DeviceStatusResponse::Status::SERVICE:
            {
                // TODO send service status to platform?
            }
            }
        }
        else if (StatusProtocol::isLastWillMessage(topic))
        {
            if (auto handler = m_gatewayModuleConnectionStatusListener.lock())
            {
                LOG(INFO) << "Device Status Service: Gateway module got disconnected";
                handler->disconnected();
            }
        }
        else
        {
            LOG(WARN) << "Device Status Service: Incorrect status channel from gateway: " << topic;
        }
    }
    else
    {
        LOG(WARN) << "Device Status Service: Status channel not parsed: " << topic;
    }
}

void DeviceStatusService::setGatewayModuleConnectionStatusListener(std::weak_ptr<ConnectionStatusListener> listener)
{
    m_gatewayModuleConnectionStatusListener = listener;
}

void DeviceStatusService::routeDeviceMessage(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    const std::string topic = StatusProtocol::routeDeviceMessage(message->getChannel(), m_gatewayKey);
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

    const std::string topic = StatusProtocol::routePlatformMessage(message->getChannel(), m_gatewayKey);
    if (topic.empty())
    {
        LOG(WARN) << "Failed to route platform message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), topic)};

    m_outboundDeviceMessageHandler.addMessage(routedMessage);
}
}
