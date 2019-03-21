/*
 * Copyright 2019 WolkAbout Technology s.r.o.
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
#include "model/DeviceStatus.h"
#include "model/Message.h"
#include "protocol/GatewayStatusProtocol.h"
#include "protocol/StatusProtocol.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"

namespace
{
const std::chrono::seconds STATUS_RESPONSE_TIMEOUT{5};
}

namespace wolkabout
{
DeviceStatusService::DeviceStatusService(std::string gatewayKey, StatusProtocol& protocol,
                                         GatewayStatusProtocol& gatewayProtocol, DeviceRepository* deviceRepository,
                                         OutboundMessageHandler& outboundPlatformMessageHandler,
                                         OutboundMessageHandler& outboundDeviceMessageHandler,
                                         std::chrono::seconds statusRequestInterval)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_gatewayProtocol{gatewayProtocol}
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

    if (m_protocol.isStatusRequestMessage(*message))
    {
        const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(topic);

        if (deviceKey.empty())
        {
            return;
        }

        sendStatusRequestForDevice(deviceKey);
    }
    else if (m_protocol.isStatusConfirmMessage(*message))
    {
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

    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(topic);

    if (m_gatewayProtocol.isLastWillMessage(*message))
    {
        if (!deviceKey.empty())
        {
            LOG(INFO) << "Device Status Service: Device got disconnected: " << deviceKey;

            // offline status if last will topic contains device key
            logDeviceStatus(deviceKey, DeviceStatus::OFFLINE);
            sendStatusUpdateForDevice(deviceKey, DeviceStatus::OFFLINE);
        }
        else    // check for list of key in payload
        {
            const auto deviceKeys = m_gatewayProtocol.extractDeviceKeysFromContent(message->getContent());

            for (const auto& key : deviceKeys)
            {
                LOG(INFO) << "Device Status Service: Device got disconnected: " << key;

                logDeviceStatus(key, DeviceStatus::OFFLINE);
                sendStatusUpdateForDevice(key, DeviceStatus::OFFLINE);
            }
        }
    }
    else if (m_gatewayProtocol.isStatusResponseMessage(*message) || m_gatewayProtocol.isStatusUpdateMessage(*message))
    {
        if (deviceKey.empty())
        {
            return;
        }

        auto statusResponse = m_gatewayProtocol.makeDeviceStatusResponse(*message);
        if (!statusResponse)
        {
            LOG(WARN) << "Device Status Service: Unable to parse device status response";
            return;
        }

        logDeviceStatus(deviceKey, statusResponse->getStatus());

        sendStatusUpdateForDevice(deviceKey, statusResponse->getStatus());
    }
    else
    {
        LOG(WARN) << "Device Status Service: Status channel not parsed: " << topic;
    }
}

const Protocol& DeviceStatusService::getProtocol() const
{
    return m_protocol;
}

const GatewayProtocol& DeviceStatusService::getGatewayProtocol() const
{
    return m_gatewayProtocol;
}

void DeviceStatusService::sendLastKnownStatusForDevice(const std::string& deviceKey)
{
    if (containsDeviceStatus(deviceKey))
    {
        auto status = getDeviceStatus(deviceKey).second;
        sendStatusUpdateForDevice(deviceKey, status);
    }
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

void DeviceStatusService::requestDevicesStatus()
{
    if (m_deviceRepository)
    {
        auto keys = m_deviceRepository->findAllDeviceKeys();

        for (const auto& key : *keys)
        {
            if (key == m_gatewayKey)
            {
                continue;
            }

            sendStatusRequestForDevice(key);
        }

        m_responseTimer.start(std::chrono::duration_cast<std::chrono::milliseconds>(m_statusResponseInterval),
                              [=] { validateDevicesStatus(); });
    }
    else
    {
        sendStatusRequestForAllDevices();
    }
}

void DeviceStatusService::validateDevicesStatus()
{
    if (!m_deviceRepository)
    {
        return;
    }

    auto keys = m_deviceRepository->findAllDeviceKeys();

    for (const auto& key : *keys)
    {
        if (key == m_gatewayKey)
        {
            continue;
        }

        if (!containsDeviceStatus(key))
        {
            // device has not reported status at all, send offline status
            logDeviceStatus(key, DeviceStatus::OFFLINE);
            sendStatusUpdateForDevice(key, DeviceStatus::OFFLINE);
            continue;
        }

        auto statusReport = getDeviceStatus(key);

        std::time_t lastReportTime = statusReport.first;
        std::time_t currentTime = std::time(nullptr);
        DeviceStatus::Status lastStatus = statusReport.second;

        if (std::difftime(lastReportTime, currentTime) > m_statusResponseInterval.count() &&
            lastStatus == DeviceStatus::Status::CONNECTED)
        {
            // device has not reported status in time and last status was CONNECTED, send offline status
            logDeviceStatus(key, DeviceStatus::OFFLINE);
            sendStatusUpdateForDevice(key, DeviceStatus::OFFLINE);
            continue;
        }
    }
}

void DeviceStatusService::sendStatusRequestForDevice(const std::string& deviceKey)
{
    std::shared_ptr<Message> message = m_gatewayProtocol.makeDeviceStatusRequestMessage(deviceKey);
    if (!message)
    {
        LOG(WARN) << "Failed to create status request message for device: " << deviceKey;
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(message);
}

void DeviceStatusService::sendStatusRequestForAllDevices()
{
    std::shared_ptr<Message> message = m_gatewayProtocol.makeDeviceStatusRequestMessage("");
    if (!message)
    {
        LOG(WARN) << "Failed to create status request message for all devices";
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(message);
}

void DeviceStatusService::sendStatusUpdateForDevice(const std::string& deviceKey, DeviceStatus status)
{
    // TODO protocol
    std::shared_ptr<Message> statusMessage =
      m_protocol.makeStatusResponseMessage(m_gatewayKey, DeviceStatus{deviceKey, status});

    if (!statusMessage)
    {
        LOG(WARN) << "Failed to create status message for device: " << deviceKey;
        return;
    }

    m_outboundPlatformMessageHandler.addMessage(statusMessage);
}

bool DeviceStatusService::containsDeviceStatus(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_deviceStatusMutex)> lg{m_deviceStatusMutex};

    return m_deviceStatuses.find(deviceKey) != m_deviceStatuses.end();
}

std::pair<std::time_t, DeviceStatus::Status> DeviceStatusService::getDeviceStatus(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_deviceStatusMutex)> lg{m_deviceStatusMutex};

    auto it = m_deviceStatuses.find(deviceKey);

    if (it != m_deviceStatuses.end())
    {
        return it->second;
    }

    return std::make_pair(0, DeviceStatus::Status::OFFLINE);
}

void DeviceStatusService::logDeviceStatus(const std::string& deviceKey, DeviceStatus::Status status)
{
    std::lock_guard<decltype(m_deviceStatusMutex)> lg{m_deviceStatusMutex};

    m_deviceStatuses[deviceKey] = std::make_pair(std::time(nullptr), status);
}

}    // namespace wolkabout
