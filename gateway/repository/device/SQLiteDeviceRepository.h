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

#include "gateway/repository/device/DeviceRepository.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>

// Forward declare the context structure for sqlite.
struct sqlite3;

namespace wolkabout::gateway
{
// This is the map in which results from an SQL query will be returned
using ColumnResult = std::map<std::uint64_t, std::vector<std::string>>;

class SQLiteDeviceRepository : public DeviceRepository
{
public:
    explicit SQLiteDeviceRepository(const std::string& connectionString = "deviceRepository.db");

    ~SQLiteDeviceRepository() override;

    bool save(const std::vector<StoredDeviceInformation>& devices) override;

    bool remove(const std::vector<std::string>& deviceKeys) override;

    bool removeAll() override;

    bool containsDevice(const std::string& deviceKey) override;

    StoredDeviceInformation get(const std::string& deviceKey) override;

    std::vector<StoredDeviceInformation> getGatewayDevices() override;

    std::chrono::milliseconds latestPlatformTimestamp() override;

private:
    bool update(const std::vector<StoredDeviceInformation>& devices);

    StoredDeviceInformation loadDeviceInformationFromRow(ColumnResult& result, std::uint64_t row);

    std::string executeSQLStatement(const std::string& sqlStatement, ColumnResult* result = nullptr);

    std::recursive_mutex m_mutex;
    sqlite3* m_db;
};
}    // namespace wolkabout::gateway

#endif    // DEVICEREPOSITORYIMPL_H
