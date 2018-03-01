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

#ifndef DEVICEREPOSITORYIMPL_H
#define DEVICEREPOSITORYIMPL_H

#include "Poco/Data/Session.h"
#include "repository/DeviceRepository.h"

#include <memory>
#include <mutex>
#include <string>

namespace wolkabout
{
class SQLiteDeviceRepository : public DeviceRepository
{
public:
    SQLiteDeviceRepository(const std::string& connectionString = "deviceRepository.db");
    virtual ~SQLiteDeviceRepository() = default;

    virtual void save(const Device& device) override;

    virtual void update(const Device& device) override;

    virtual void remove(const std::string& deviceKey) override;

    virtual std::unique_ptr<Device> findByDeviceKey(const std::string& deviceKey) override;

    virtual std::unique_ptr<std::vector<std::string>> findAllDeviceKeys() override;

    virtual bool containsDeviceWithKey(const std::string& deviceKey) override;

private:
    std::recursive_mutex m_mutex;
    std::unique_ptr<Poco::Data::Session> m_session;
};
}    // namespace wolkabout

#endif    // DEVICEREPOSITORYIMPL_H
