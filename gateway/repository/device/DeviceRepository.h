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

#ifndef DEVICEREPOSITORY_H
#define DEVICEREPOSITORY_H

#include "core/model/messages/RegisteredDevicesResponseMessage.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
namespace gateway
{
class DeviceRepository
{
public:
    virtual ~DeviceRepository() = default;

    virtual bool save(std::chrono::milliseconds timestamp, const RegisteredDeviceInformation& device) = 0;

    virtual bool remove(const std::string& deviceKey) = 0;

    virtual bool removeAll() = 0;

    virtual bool containsDeviceKey(const std::string& deviceKey) = 0;

    virtual std::chrono::milliseconds latestTimestamp() = 0;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // DEVICEREPOSITORY_H
