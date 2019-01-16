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

#include "model/DetailedDevice.h"

#include <string>

namespace wolkabout
{
class Device : public DetailedDevice
{
public:
    Device() = default;
    Device(std::string key, std::string password, std::string protocol);
    Device(std::string key, std::string password, DeviceManifest deviceManifest);

    const std::string& getKey() const;
    const std::string& getPassword() const;
};
}    // namespace wolkabout

#endif    // DEVICE_H
