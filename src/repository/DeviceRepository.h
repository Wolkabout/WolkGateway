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

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Device;
class DeviceRepository
{
public:
    virtual ~DeviceRepository() = default;

    virtual void save(const Device& device) = 0;

    virtual void update(const Device& device) = 0;

    virtual void remove(const std::string& devicekey) = 0;

    virtual std::shared_ptr<Device> findByDeviceKey(const std::string& key) = 0;

    virtual std::shared_ptr<std::vector<std::string>> findAllDeviceKeys() = 0;

    virtual bool containsDeviceWithKey(const std::string& deviceKey) = 0;
};
}    // namespace wolkabout

#endif    // DEVICEREPOSITORY_H
