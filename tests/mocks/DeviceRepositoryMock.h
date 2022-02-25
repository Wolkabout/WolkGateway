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

#ifndef WOLKGATEWAY_DEVICEREPOSITORYMOCK_H
#define WOLKGATEWAY_DEVICEREPOSITORYMOCK_H

#include "gateway/repository/device/DeviceRepository.h"

#include <gmock/gmock.h>

using namespace wolkabout;
using namespace wolkabout::gateway;

class DeviceRepositoryMock : public DeviceRepository
{
public:
    MOCK_METHOD(bool, save, (const std::vector<StoredDeviceInformation>&));
    MOCK_METHOD(bool, remove, (const std::vector<std::string>&));
    MOCK_METHOD(bool, removeAll, ());
    MOCK_METHOD(bool, containsDevice, (const std::string&));
    MOCK_METHOD(StoredDeviceInformation, get, (const std::string&));
    MOCK_METHOD(std::vector<StoredDeviceInformation>, getGatewayDevices, ());
    MOCK_METHOD(std::chrono::milliseconds, latestPlatformTimestamp, ());
};

#endif    // WOLKGATEWAY_DEVICEREPOSITORYMOCK_H
