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

class DeviceStatusService : public PlatformMessageListener, public ConnectionStatusListener
{
public:
    DeviceStatusService(std::string gatewayKey, StatusProtocol& protocol,
                        OutboundMessageHandler& outboundPlatformMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    const Protocol& getProtocol() const override;

protected:
    void sendStatusUpdateForDevice(const std::string& deviceKey, DeviceStatus::Status status);
    void sendStatusResponseForDevice(const std::string& deviceKey, DeviceStatus::Status status);

    const std::string m_gatewayKey;
    StatusProtocol& m_protocol;

private:
    virtual void requestDeviceStatus(const std::string& deviceKey) = 0;

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
};
}    // namespace wolkabout

#endif    // DEVICESTATUSSERVICE_H
