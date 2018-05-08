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
#include "model/DeviceStatusResponse.h"
#include "model/Message.h"
#include "protocol/GatewayStatusProtocol.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"

namespace
{
const std::chrono::seconds STATUS_RESPONSE_TIMEOUT{5};
}

namespace wolkabout
{
DeviceStatusService::DeviceStatusService(std::string gatewayKey, GatewayStatusProtocol& protocol,
                                         DeviceRepository& deviceRepository,
                                         OutboundMessageHandler& outboundPlatformMessageHandler,
                                         OutboundMessageHandler& outboundDeviceMessageHandler,
                                         std::chrono::seconds statusRequestInterval)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_deviceRepository{deviceRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_statusRequestInterval{statusRequestInterval}
, m_statusResponseInterval{STATUS_RESPONSE_TIMEOUT}
{
}

void DeviceStatusService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = message->getChannel();

    if (!m_protocol.isMessageFromPlatform(*message))
    {
        LOG(WARN) << "DeviceRegistrationService: Ignoring message on channel '" << topic
                  << "'. Message not from platform.";
        return;
    }

    if (m_protocol.isStatusRequestMessage(*message))
    {
        // TODO platform does not support status request message,
        // instead it sends OK response when device status is sent
        // routePlatformMessage(message);
    }
    else
    {
        LOG(WARN) << "Message channel not parsed: " << topic;
    }
}

void DeviceStatusService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = message->getChannel();

    if (!m_protocol.isMessageToPlatform(*message))
    {
        LOG(WARN) << "DeviceStatusService: Ignoring message on channel '" << topic
                  << "'. Message not intended for platform.";
        return;
    }

    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(topic);

    if (m_protocol.isLastWillMessage(*message))
    {
        if (!deviceKey.empty())
        {
            if (deviceKey == m_gatewayKey)
            {
                LOG(INFO) << "Device Status Service: Gateway module got disconnected";
                if (auto handler = m_gatewayModuleConnectionStatusListener.lock())
                {
                    handler->disconnected();
                }
            }
            else
            {
                LOG(INFO) << "Device Status Service: Device got disconnected: " << deviceKey;

                // offline status if last will topic contains device key
                sendStatusResponseForDevice(deviceKey, DeviceStatus::OFFLINE);
            }
        }
        else    // check for list of key in payload
        {
            const auto deviceKeys = m_protocol.extractDeviceKeysFromContent(message->getContent());

            for (const auto& key : deviceKeys)
            {
                LOG(INFO) << "Device Status Service: Device got disconnected: " << key;

                sendStatusResponseForDevice(key, DeviceStatus::OFFLINE);
            }
        }
    }
    else if (m_protocol.isStatusResponseMessage(*message))
    {
        if (deviceKey.empty())
        {
            return;
        }

        if (deviceKey == m_gatewayKey)
        {
            auto deviceStatusResponse = m_protocol.makeDeviceStatusResponse(*message);
            if (!deviceStatusResponse)
            {
                LOG(WARN) << "Device Status Service: Could not parse message: " << topic << ", "
                          << message->getContent();
                return;
            }

            switch (deviceStatusResponse->getStatus())
            {
            case DeviceStatus::CONNECTED:
            {
                if (auto handler = m_gatewayModuleConnectionStatusListener.lock())
                {
                    LOG(INFO) << "Device Status Service: Gateway module connected";
                    handler->connected();
                }
                break;
            }
            case DeviceStatus::SLEEP:
            case DeviceStatus::OFFLINE:
            {
                if (auto handler = m_gatewayModuleConnectionStatusListener.lock())
                {
                    LOG(INFO) << "Device Status Service: Gateway module got disconnected";
                    handler->disconnected();
                }
                break;
            }
            case DeviceStatus::SERVICE:
            {
                // TODO send service status to platform?
            }
            }
        }
        else
        {
            auto statusResponse = m_protocol.makeDeviceStatusResponse(*message);
            if (!statusResponse)
            {
                LOG(WARN) << "Device Status Service: Unable to parse device status response";
                return;
            }

            logDeviceStatus(deviceKey, statusResponse->getStatus());

            routeDeviceMessage(message);
        }
    }
    else
    {
        LOG(WARN) << "Device Status Service: Status channel not parsed: " << topic;
    }
}

const GatewayProtocol& DeviceStatusService::getProtocol() const
{
    return m_protocol;
}

void DeviceStatusService::setGatewayModuleConnectionStatusListener(std::weak_ptr<ConnectionStatusListener> listener)
{
    m_gatewayModuleConnectionStatusListener = listener;
}

void DeviceStatusService::connected()
{
    requestDevicesStatus();

    m_requestTimer.run(std::chrono::duration_cast<std::chrono::milliseconds>(m_statusRequestInterval),
                       [=] { requestDevicesStatus(); });
}

void DeviceStatusService::disconnected()
{
    m_requestTimer.stop();
    m_responseTimer.stop();
}

void DeviceStatusService::routeDeviceMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = m_protocol.routeDeviceMessage(message->getChannel(), m_gatewayKey);
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
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = m_protocol.routePlatformMessage(message->getChannel(), m_gatewayKey);
    if (topic.empty())
    {
        LOG(WARN) << "Failed to route platform message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), topic)};

    m_outboundDeviceMessageHandler.addMessage(routedMessage);
}

void DeviceStatusService::requestDevicesStatus()
{
    auto keys = m_deviceRepository.findAllDeviceKeys();

    for (const auto& key : *keys)
    {
        if (key != m_gatewayKey)
        {
            sendStatusRequestForDevice(key);
        }
    }

    m_responseTimer.start(std::chrono::duration_cast<std::chrono::milliseconds>(m_statusResponseInterval),
                          [=] { validateDevicesStatus(); });
}

void DeviceStatusService::validateDevicesStatus()
{
    auto keys = m_deviceRepository.findAllDeviceKeys();

    for (const auto& key : *keys)
    {
        if (key == m_gatewayKey)
        {
            continue;
        }

        auto it = m_deviceStatuses.find(key);

        if (it == m_deviceStatuses.end())
        {
            // device has not reported status at all, send offline status
            sendStatusResponseForDevice(key, DeviceStatus::OFFLINE);
            continue;
        }

        std::time_t lastReportTime = it->second.first;
        std::time_t currentTime = std::time(nullptr);
        DeviceStatus lastStatus = it->second.second;

        if (std::difftime(lastReportTime, currentTime) > m_statusResponseInterval.count() &&
            lastStatus == DeviceStatus::CONNECTED)
        {
            // device has not reported status in time and last status was CONNECTED, send offline status
            sendStatusResponseForDevice(key, DeviceStatus::OFFLINE);
            continue;
        }
    }
}

void DeviceStatusService::sendStatusRequestForDevice(const std::string& deviceKey)
{
    std::shared_ptr<Message> message = m_protocol.makeDeviceStatusRequestMessage(deviceKey);
    if (!message)
    {
        LOG(WARN) << "Failed to create status request message for device: " << deviceKey;
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(message);
}

void DeviceStatusService::sendStatusResponseForDevice(const std::string& deviceKey, DeviceStatus status)
{
    std::shared_ptr<Message> statusMessage = m_protocol.makeMessage(m_gatewayKey, deviceKey, status);

    if (!statusMessage)
    {
        LOG(WARN) << "Failed to create status message for device: " << deviceKey;
        return;
    }

    logDeviceStatus(deviceKey, status);

    m_outboundPlatformMessageHandler.addMessage(statusMessage);
}

void DeviceStatusService::logDeviceStatus(const std::string& deviceKey, DeviceStatus status)
{
    m_deviceStatuses[deviceKey] = std::make_pair(std::time(nullptr), status);
}

}    // namespace wolkabout
