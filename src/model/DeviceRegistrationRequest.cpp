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

#include "model/DeviceRegistrationRequest.h"
#include "model/Device.h"
#include "model/DeviceManifest.h"

#include <string>
#include <utility>

namespace wolkabout
{
DeviceRegistrationRequest::DeviceRegistrationRequest(std::string deviceName, std::string deviceKey,
                                                     DeviceManifest deviceManifest)
: m_device(std::move(deviceName), std::move(deviceKey), std::move(deviceManifest))
{
}

DeviceRegistrationRequest::DeviceRegistrationRequest(Device device) : m_device(std::move(device)) {}

const std::string& DeviceRegistrationRequest::getDeviceName() const
{
    return m_device.getName();
}

const std::string& DeviceRegistrationRequest::getDeviceKey() const
{
    return m_device.getKey();
}

const DeviceManifest& DeviceRegistrationRequest::getManifest() const
{
    return m_device.getManifest();
}
}    // namespace wolkabout
