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

#include "Device.h"

#include <string>
#include <utility>

namespace wolkabout
{
Device::Device(std::string name, std::string key, DeviceManifest deviceManifest)
: Device(std::move(name), std::move(key), "", std::move(deviceManifest))
{
}

Device::Device(std::string name, std::string key, std::string password, DeviceManifest deviceManifest)
: m_name(std::move(name))
, m_key(std::move(key))
, m_password(std::move(password))
, m_deviceManifest(std::move(deviceManifest))
{
}

const std::string& Device::getName() const
{
    return m_name;
}

const std::string& Device::getKey() const
{
    return m_key;
}

const std::string& Device::getPassword() const
{
    return m_password;
}

const DeviceManifest& Device::getManifest() const
{
    return m_deviceManifest;
}

std::vector<std::string> Device::getActuatorReferences() const
{
    std::vector<std::string> actuatorReferences(m_deviceManifest.getActuators().size());
    for (const ActuatorManifest& actuatorManifest : m_deviceManifest.getActuators())
    {
        actuatorReferences.push_back(actuatorManifest.getReference());
    }

    return actuatorReferences;
}

bool Device::operator==(Device& rhs) const
{
    if (m_key != rhs.m_key || m_name != rhs.m_name || m_password != rhs.m_password ||
        m_deviceManifest != rhs.m_deviceManifest)
    {
        return false;
    }

    return true;
}

bool Device::operator!=(Device& rhs) const
{
    return !(*this == rhs);
}
}    // namespace wolkabout
