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

#ifndef DEVICE_H
#define DEVICE_H

#include "model/DeviceManifest.h"

#include <string>

namespace wolkabout
{
class Device
{
public:
    Device() = default;
    Device(std::string name, std::string key, DeviceManifest deviceManifest);
    Device(std::string name, std::string key, std::string password, DeviceManifest deviceManifest);

    virtual ~Device() = default;

    const std::string& getName() const;
    const std::string& getKey() const;
    const std::string& getPassword() const;

    const DeviceManifest& getManifest() const;

    std::vector<std::string> getActuatorReferences() const;

    bool operator==(Device& rhs) const;
    bool operator!=(Device& rhs) const;

private:
    std::string m_name;
    std::string m_key;
    std::string m_password;

    DeviceManifest m_deviceManifest;
};
}    // namespace wolkabout

#endif    // DEVICE_H
