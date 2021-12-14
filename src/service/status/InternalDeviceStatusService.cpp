/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#include "InternalDeviceStatusService.h"

#include "OutboundMessageHandler.h"
#include "core/model/Message.h"
#include "core/utilities/Logger.h"
#include "protocol/GatewayStatusProtocol.h"
#include "repository/DeviceRepository.h"

#include <algorithm>

namespace
{
const std::chrono::seconds STATUS_RESPONSE_TIMEOUT{5};
}

namespace wolkabout
{
InternalDeviceStatusService::InternalDeviceStatusService(std::string gatewayKey, StatusProtocol& protocol,
                                                         GatewayStatusProtocol& gatewayProtocol,
                                                         DeviceRepository* deviceRepository,
                                                         OutboundMessageHandler& outboundPlatformMessageHandler,
                                                         OutboundMessageHandler& outboundDeviceMessageHandler,
                                                         std::chrono::seconds statusRequestInterval)
: DeviceStatusService(gatewayKey, protocol, outboundPlatformMessageHandler)
, m_gatewayProtocol{gatewayProtocol}
, m_deviceRepository{deviceRepository}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_statusRequestInterval{statusRequestInterval}
, m_statusResponseInterval{STATUS_RESPONSE_TIMEOUT}
{
}

void InternalDeviceStatusService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = message->getChannel();

    const std::string deviceKey = m_gatewayProtocol.extractDeviceKeyFromChannel(topic);

    if (m_gatewayProtocol.isLastWillMessage(*message))
    {
        if (!deviceKey.empty())
        {
            LOG(INFO) << "Device Status Service: Device got disconnected: " << deviceKey;

            // offline status if last will topic contains device key
            logDeviceStatus(deviceKey, DeviceStatus::Status::OFFLINE);
            sendStatusUpdateForDevice(deviceKey, DeviceStatus::Status::OFFLINE);
        }
        else    // check for list of key in payload
        {
            const auto deviceKeys = m_gatewayProtocol.extractDeviceKeysFromContent(message->getContent());

            for (const auto& key : deviceKeys)
            {
                LOG(INFO) << "Device Status Service: Device got disconnected: " << key;

                logDeviceStatus(key, DeviceStatus::Status::OFFLINE);
                sendStatusUpdateForDevice(key, DeviceStatus::Status::OFFLINE);
            }
        }
    }
    else if (m_gatewayProtocol.isStatusResponseMessage(*message))
    {
        auto statusResponse = m_gatewayProtocol.makeDeviceStatusResponse(*message);
        if (!statusResponse)
        {
            LOG(WARN) << "Device Status Service: Unable to parse device status response";
            return;
        }

        if (statusResponse->getDeviceKey().empty())
        {
            LOG(WARN) << "Device Status Service: Missing device key in device status response";
            return;
        }

        logDeviceStatus(statusResponse->getDeviceKey(), statusResponse->getStatus());
        if (isInSelfRequested(statusResponse->getDeviceKey()))
        {
            removeFromSelfRequested(statusResponse->getDeviceKey());
            sendStatusUpdateForDevice(statusResponse->getDeviceKey(), statusResponse->getStatus());
        }
        else
        {
            sendStatusResponseForDevice(statusResponse->getDeviceKey(), statusResponse->getStatus());
        }
    }
    else if (m_gatewayProtocol.isStatusUpdateMessage(*message))
    {
        auto statusUpdate = m_gatewayProtocol.makeDeviceStatusUpdate(*message);
        if (!statusUpdate)
        {
            LOG(WARN) << "Device Status Service: Unable to parse device status update";
            return;
        }

        if (statusUpdate->getDeviceKey().empty())
        {
            LOG(WARN) << "Device Status Service: Missing device key in device status update";
            return;
        }

        logDeviceStatus(statusUpdate->getDeviceKey(), statusUpdate->getStatus());
        sendStatusUpdateForDevice(statusUpdate->getDeviceKey(), statusUpdate->getStatus());
    }
    else
    {
        LOG(WARN) << "Device Status Service: Status channel not parsed: " << topic;
    }
}

const GatewayProtocol& InternalDeviceStatusService::getGatewayProtocol() const
{
    return m_gatewayProtocol;
}

void InternalDeviceStatusService::connected()
{
    requestDevicesStatus();

    m_requestTimer.run(std::chrono::duration_cast<std::chrono::milliseconds>(m_statusRequestInterval),
                       [=] { requestDevicesStatus(); });
}

void InternalDeviceStatusService::disconnected()
{
    m_requestTimer.stop();
    m_responseTimer.stop();
}

void InternalDeviceStatusService::sendLastKnownStatusForDevice(const std::string& deviceKey)
{
    if (containsDeviceStatus(deviceKey))
    {
        auto status = getDeviceStatus(deviceKey).second;
        sendStatusUpdateForDevice(deviceKey, status);
    }
}

void InternalDeviceStatusService::requestDeviceStatus(const std::string& deviceKey)
{
    sendStatusRequestForDevice(deviceKey);
}

void InternalDeviceStatusService::requestDevicesStatus()
{
    if (m_deviceRepository)
    {
        auto keys = m_deviceRepository->findAllDeviceKeys();

        clearSelfRequest();

        for (const auto& key : *keys)
        {
            if (key == m_gatewayKey)
            {
                continue;
            }

            addToSelfRequest(key);
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

void InternalDeviceStatusService::validateDevicesStatus()
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
            logDeviceStatus(key, DeviceStatus::Status::OFFLINE);
            removeFromSelfRequested(key);
            sendStatusUpdateForDevice(key, DeviceStatus::Status::OFFLINE);
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
            logDeviceStatus(key, DeviceStatus::Status::OFFLINE);
            removeFromSelfRequested(key);
            sendStatusUpdateForDevice(key, DeviceStatus::Status::OFFLINE);
            continue;
        }
    }
}

void InternalDeviceStatusService::sendStatusRequestForDevice(const std::string& deviceKey)
{
    std::shared_ptr<Message> message = m_gatewayProtocol.makeDeviceStatusRequestMessage(deviceKey);
    if (!message)
    {
        LOG(WARN) << "Failed to create status request message for device: " << deviceKey;
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(message);
}

void InternalDeviceStatusService::sendStatusRequestForAllDevices()
{
    std::shared_ptr<Message> message = m_gatewayProtocol.makeDeviceStatusRequestMessage("");
    if (!message)
    {
        LOG(WARN) << "Failed to create status request message for all devices";
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(message);
}

bool InternalDeviceStatusService::containsDeviceStatus(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_deviceStatusMutex)> lg{m_deviceStatusMutex};

    return m_deviceStatuses.find(deviceKey) != m_deviceStatuses.end();
}

std::pair<std::time_t, DeviceStatus::Status> InternalDeviceStatusService::getDeviceStatus(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_deviceStatusMutex)> lg{m_deviceStatusMutex};

    auto it = m_deviceStatuses.find(deviceKey);

    if (it != m_deviceStatuses.end())
    {
        return it->second;
    }

    return std::make_pair(0, DeviceStatus::Status::OFFLINE);
}

void InternalDeviceStatusService::logDeviceStatus(const std::string& deviceKey, DeviceStatus::Status status)
{
    std::lock_guard<decltype(m_deviceStatusMutex)> lg{m_deviceStatusMutex};

    m_deviceStatuses[deviceKey] = std::make_pair(std::time(nullptr), status);
}

void InternalDeviceStatusService::clearSelfRequest()
{
    m_selfRequestedDevices.clear();
}

void InternalDeviceStatusService::addToSelfRequest(const std::string& key)
{
    m_selfRequestedDevices.emplace_back(key);
}

bool InternalDeviceStatusService::isInSelfRequested(const std::string& key)
{
    const auto it = std::find(m_selfRequestedDevices.cbegin(), m_selfRequestedDevices.cend(), key);
    return it != m_selfRequestedDevices.cend();
}

void InternalDeviceStatusService::removeFromSelfRequested(const std::string& key)
{
    const auto it = std::find(m_selfRequestedDevices.cbegin(), m_selfRequestedDevices.cend(), key);
    if (it != m_selfRequestedDevices.cend())
        m_selfRequestedDevices.erase(it);
}
}    // namespace wolkabout
