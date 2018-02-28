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
#include <memory>
#include <string>

namespace wolkabout
{
class DeviceManager;
class OutboundMessageHandler;
class Message;
class DeviceRegistrationRequestDto;
class DeviceRegistrationResponseDto;

class DeviceRegistrationService : public DeviceMessageListener, public PlatformMessageListener
{
public:
    DeviceRegistrationService(std::string gatewayKey, DeviceManager& deviceManager,
                              std::shared_ptr<OutboundMessageHandler> outboundPlatformMessageHandler,
                              std::shared_ptr<OutboundMessageHandler> outboundDeviceMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

private:
    void handleDeviceRegistrationRequest(std::shared_ptr<DeviceRegistrationRequestDto> request);
    void handleGatewayRegistrationRequest(std::shared_ptr<DeviceRegistrationRequestDto> request);
    void handleDeviceRegistrationResponse(std::shared_ptr<DeviceRegistrationResponseDto> response);
    void handleGatewayRegistrationResponse(std::shared_ptr<DeviceRegistrationResponseDto> response);

    const std::string m_gatewayKey;

    DeviceManager& m_deviceManager;
    std::shared_ptr<OutboundMessageHandler> m_outboundPlatformMessageHandler;
    std::shared_ptr<OutboundMessageHandler> m_outboundDeviceMessageHandler;
};
}

#endif
