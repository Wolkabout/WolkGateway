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

#include "InboundDeviceMessageHandler.h"
#include "InboundPlatformMessageHandler.h"
#include "model/DeviceRegistrationRequest.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace wolkabout
{
class Device;
class DeviceRepository;
class OutboundMessageHandler;
class Message;
class DeviceRegistrationResponse;

class DeviceRegistrationService : public DeviceMessageListener, public PlatformMessageListener
{
public:
    DeviceRegistrationService(std::string gatewayKey, DeviceRepository& deviceRepository,
                              OutboundMessageHandler& outboundDeviceMessageHandler,
                              OutboundMessageHandler& outboundPlatformMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    void onDeviceRegistered(std::function<void(const std::string& deviceKey, bool isGateway)> onDeviceRegistered);

protected:
    void invokeOnDeviceRegisteredListener(const std::string& deviceKey, bool isGateway) const;

private:
    void handleDeviceRegistrationRequest(const std::string& deviceKey, const DeviceRegistrationRequest& request);
    void handleDeviceReregistrationRequest();

    void handleDeviceRegistrationResponse(const std::string& deviceKey, const DeviceRegistrationResponse& response);

    void addToPostponedDeviceRegistartionRequests(const std::string& deviceKey,
                                                  const DeviceRegistrationRequest& request);

    const std::string m_gatewayKey;

    DeviceRepository& m_deviceRepository;
    OutboundMessageHandler& m_outboundDeviceMessageHandler;
    OutboundMessageHandler& m_outboundPlatformMessageHandler;

    std::function<void(const std::string& deviceKey, bool isGateway)> m_onDeviceRegistered;

    std::recursive_mutex m_devicesAwaitingRegistrationResponseMutex;
    std::map<std::string, std::unique_ptr<Device>> m_devicesAwaitingRegistrationResponse;

    std::mutex m_devicesWithPostponedRegistrationMutex;
    std::map<std::string, std::unique_ptr<DeviceRegistrationRequest>> m_devicesWithPostponedRegistration;
};
}    // namespace wolkabout

#endif
