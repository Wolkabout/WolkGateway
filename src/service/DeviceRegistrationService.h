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
#include "model/DeviceRegistrationRequestDto.h"

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
class DeviceRegistrationResponseDto;

class DeviceRegistrationService : public DeviceMessageListener, public PlatformMessageListener
{
public:
    DeviceRegistrationService(std::string gatewayKey, DeviceRepository& deviceRepository,
                              OutboundMessageHandler& outboundPlatformMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    void onGatewayRegistered(std::function<void()> callback);

private:
    void handleDeviceRegistrationRequest(const std::string& deviceKey, const DeviceRegistrationRequestDto& request);
    void handleDeviceReregistrationRequest();

    void handleDeviceRegistrationResponse(const std::string& deviceKey, const DeviceRegistrationResponseDto& response);

    void addToPostponedDeviceRegistartionRequests(const std::string& deviceKey,
                                                  const DeviceRegistrationRequestDto& request);

    const std::string m_gatewayKey;

    DeviceRepository& m_deviceRepository;
    OutboundMessageHandler& m_outboundPlatformMessageHandler;

    std::function<void()> m_gatewayRegisteredCallback;

    std::mutex m_devicesAwaitingRegistrationResponseMutex;
    std::map<std::string, std::unique_ptr<Device>> m_devicesAwaitingRegistrationResponse;

    std::mutex m_devicesWithPostponedRegistrationMutex;
    std::map<std::string, std::unique_ptr<DeviceRegistrationRequestDto>> m_devicesWithPostponedRegistration;
};
}    // namespace wolkabout

#endif
