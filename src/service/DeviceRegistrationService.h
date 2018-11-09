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

#ifndef DEVICEREGISTRATIONSERVICE_H
#define DEVICEREGISTRATIONSERVICE_H

#include "GatewayInboundDeviceMessageHandler.h"
#include "GatewayInboundPlatformMessageHandler.h"
#include "OutboundRetryMessageHandler.h"
#include "model/DeviceRegistrationRequest.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace wolkabout
{
class DetailedDevice;
class DeviceRepository;
class DeviceRegistrationResponse;
class GatewayDeviceRegistrationProtocol;
class Message;
class OutboundMessageHandler;

class DeviceRegistrationService : public DeviceMessageListener, public PlatformMessageListener
{
public:
    DeviceRegistrationService(std::string gatewayKey, GatewayDeviceRegistrationProtocol& protocol,
                              DeviceRepository& deviceRepository,
                              OutboundMessageHandler& outboundPlatformMessageHandler,
                              OutboundMessageHandler& outboundDeviceMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    const GatewayProtocol& getProtocol() const override;

    void onDeviceRegistered(std::function<void(const std::string& deviceKey, bool isGateway)> onDeviceRegistered);

    void deleteDevicesOtherThan(const std::vector<std::string>& devicesKeys);

    void registerDevice(const DetailedDevice& device);

protected:
    void invokeOnDeviceRegisteredListener(const std::string& deviceKey, bool isGateway) const;

private:
    void handleDeviceRegistrationRequest(const std::string& deviceKey, const DeviceRegistrationRequest& request);
    void handleDeviceReregistrationRequest();

    void handleDeviceRegistrationResponse(const std::string& deviceKey, const DeviceRegistrationResponse& response);

    void addToPostponedDeviceRegistartionRequests(const std::string& deviceKey,
                                                  const DeviceRegistrationRequest& request);

    const std::string m_gatewayKey;
    GatewayDeviceRegistrationProtocol& m_protocol;

    DeviceRepository& m_deviceRepository;

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    OutboundMessageHandler& m_outboundDeviceMessageHandler;

    OutboundRetryMessageHandler m_platformRetryMessageHandler;

    std::function<void(const std::string& deviceKey, bool isGateway)> m_onDeviceRegistered;

    std::recursive_mutex m_devicesAwaitingRegistrationResponseMutex;
    std::map<std::string, std::unique_ptr<DetailedDevice>> m_devicesAwaitingRegistrationResponse;

    std::mutex m_devicesWithPostponedRegistrationMutex;
    std::map<std::string, std::unique_ptr<DeviceRegistrationRequest>> m_devicesWithPostponedRegistration;
};
}    // namespace wolkabout

#endif
