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

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace wolkabout
{
class AlarmTemplate;
class ActuatorTemplate;
class SensorTemplate;
class ConfigurationTemplate;
class DeviceTemplate;

class SQLiteDeviceRepository : public DeviceRepository
{
public:
    SQLiteDeviceRepository(const std::string& connectionString = "deviceRepository.db");
    virtual ~SQLiteDeviceRepository() = default;

    void save(const DetailedDevice& device) override;

    void remove(const std::string& deviceKey) override;

    void removeAll() override;

    std::unique_ptr<DetailedDevice> findByDeviceKey(const std::string& deviceKey) override;

    std::unique_ptr<std::vector<std::string>> findAllDeviceKeys() override;

    bool containsDeviceWithKey(const std::string& deviceKey) override;

private:
    static std::string calculateSha256(const AlarmTemplate& alarmTemplate);
    static std::string calculateSha256(const ActuatorTemplate& actuatorTemplate);
    static std::string calculateSha256(const SensorTemplate& sensorTemplate);
    static std::string calculateSha256(const ConfigurationTemplate& configurationTemplate);
    static std::string calculateSha256(const std::pair<std::string, std::string>& typeParameter);
    static std::string calculateSha256(const std::pair<std::string, bool>& firmwareUpdateParameter);
    static std::string calculateSha256(const DeviceTemplate& deviceTemplate);

    void update(const DetailedDevice& device);

    std::recursive_mutex m_mutex;
    std::unique_ptr<Poco::Data::Session> m_session;
};
}    // namespace wolkabout

#endif    // DEVICEREPOSITORYIMPL_H
