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

#ifndef WOLKGATEWAY_EXISTINGDEVICEREPOSITORYMOCK_H
#define WOLKGATEWAY_EXISTINGDEVICEREPOSITORYMOCK_H

#include "gateway/repository/existing_device/ExistingDevicesRepository.h"

#include <gmock/gmock.h>

using namespace wolkabout::gateway;

class ExistingDevicesRepositoryMock : public ExistingDevicesRepository
{
public:
    MOCK_METHOD(void, addDeviceKey, (const std::string&));
    MOCK_METHOD(std::vector<std::string>, getDeviceKeys, ());
};

#endif    // WOLKGATEWAY_EXISTINGDEVICEREPOSITORYMOCK_H
