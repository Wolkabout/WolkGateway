/**
 * Copyright 2022 Wolkabout Technology s.r.o.
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

#ifndef WOLKGATEWAY_DEVICESSERVICEMOCK_H
#define WOLKGATEWAY_DEVICESSERVICEMOCK_H

#include "gateway/service/devices/DevicesService.h"

#include <gmock/gmock.h>

using namespace wolkabout;
using namespace wolkabout::gateway;

class DevicesServiceMock : public DevicesService
{
public:
    DevicesServiceMock(const std::string& gatewayKey, RegistrationProtocol& platformRegistrationProtocol,
                       OutboundMessageHandler& outboundPlatformMessageHandler,
                       OutboundRetryMessageHandler& outboundPlatformRetryMessageHandler,
                       std::shared_ptr<GatewayRegistrationProtocol> localRegistrationProtocol = nullptr,
                       std::shared_ptr<OutboundMessageHandler> outboundDeviceMessageHandler = nullptr,
                       std::shared_ptr<DeviceRepository> deviceRepository = nullptr)
    : DevicesService(gatewayKey, platformRegistrationProtocol, outboundPlatformMessageHandler,
                     outboundPlatformRetryMessageHandler, std::move(localRegistrationProtocol),
                     std::move(outboundDeviceMessageHandler), std::move(deviceRepository))
    {
    }
    MOCK_METHOD(void, updateDeviceCache, ());
    MOCK_METHOD(bool, sendOutRegisteredDevicesRequest,
                (RegisteredDevicesRequestParameters, RegisteredDevicesRequestCallback));
};

#endif    // WOLKGATEWAY_DEVICESSERVICEMOCK_H
