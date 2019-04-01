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

#ifndef DEVICESTATUSSERVICE_H
#define DEVICESTATUSSERVICE_H

#include "ConnectionStatusListener.h"
#include "InboundDeviceMessageHandler.h"
#include "InboundPlatformMessageHandler.h"
#include "model/DeviceStatus.h"
#include "utilities/Timer.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace wolkabout
{
class DeviceRepository;
class ConnectionStatusListener;
class GatewayStatusProtocol;
class OutboundMessageHandler;
class StatusProtocol;

class DeviceStatusService : public DeviceMessageListener,
                            public PlatformMessageListener,
                            public ConnectionStatusListener
{
public:
    DeviceStatusService(std::string gatewayKey, StatusProtocol& protocol, GatewayStatusProtocol& gatewayProtocol,
                        DeviceRepository* deviceRepository, OutboundMessageHandler& outboundPlatformMessageHandler,
                        OutboundMessageHandler& outboundDeviceMessageHandler,
                        std::chrono::seconds statusRequestInterval);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    const Protocol& getProtocol() const override;

    const GatewayProtocol& getGatewayProtocol() const override;

    void sendLastKnownStatusForDevice(const std::string& deviceKey);

    void connected() override;
    void disconnected() override;

private:
    void requestDevicesStatus();
    void validateDevicesStatus();

    void sendStatusRequestForDevice(const std::string& deviceKey);
    void sendStatusRequestForAllDevices();
    void sendStatusResponseForDevice(const std::string& deviceKey, DeviceStatus::Status status);
    void sendStatusUpdateForDevice(const std::string& deviceKey, DeviceStatus::Status status);

    bool containsDeviceStatus(const std::string& deviceKey);
    std::pair<std::time_t, DeviceStatus::Status> getDeviceStatus(const std::string& deviceKey);
    void logDeviceStatus(const std::string& deviceKey, DeviceStatus::Status status);

    const std::string m_gatewayKey;
    StatusProtocol& m_protocol;
    GatewayStatusProtocol& m_gatewayProtocol;

    DeviceRepository* m_deviceRepository;

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    OutboundMessageHandler& m_outboundDeviceMessageHandler;

    const std::chrono::seconds m_statusRequestInterval;
    const std::chrono::seconds m_statusResponseInterval;
    Timer m_requestTimer;
    Timer m_responseTimer;

    std::mutex m_deviceStatusMutex;
    std::map<std::string, std::pair<std::time_t, DeviceStatus::Status>> m_deviceStatuses;
};
}    // namespace wolkabout

#endif    // DEVICESTATUSSERVICE_H
