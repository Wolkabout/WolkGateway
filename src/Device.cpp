/*
 * Copyright 2017 WolkAbout Technology s.r.o.
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

#include "Device.h"

#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
Device::Device(std::string deviceKey, std::string devicePassword, std::vector<std::string> actuatorReferences)
: m_deviceKey(std::move(deviceKey))
, m_devicePassword(std::move(devicePassword))
, m_actuatorReferences(std::move(actuatorReferences))
{
}

Device& Device::setDeviceKey(const std::string& key)
{
    m_deviceKey = key;

    return *this;
}

const std::string& Device::getDeviceKey()
{
    return m_deviceKey;
}

Device& Device::setDevicePassword(const std::string& password)
{
    m_devicePassword = password;

    return *this;
}

const std::string& Device::getDevicePassword()
{
    return m_devicePassword;
}

Device& Device::setActuatorReferences(const std::vector<std::string>& actuators)
{
    m_actuatorReferences = actuators;

    return *this;
}

const std::vector<std::string> Device::getActuatorReferences()
{
    return m_actuatorReferences;
}
}
