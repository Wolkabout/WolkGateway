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

#include "gateway/repository/device/SQLiteDeviceRepository.h"

#include "core/model/Device.h"
#include "core/utility/ByteUtils.h"
#include "core/utility/Logger.h"

#include <mutex>
#include <sqlite3.h>
#include <string>

using namespace wolkabout::legacy;

namespace wolkabout::gateway
{
// Here are some create table instructions
const std::string CREATE_DEVICE_TABLE =
  "CREATE TABLE IF NOT EXISTS Device (ID INTEGER PRIMARY KEY AUTOINCREMENT, DeviceKey TEXT NOT NULL UNIQUE, BelongsTo "
  "TEXT CHECK( BelongsTo IN ('Platform', 'Gateway')), Timestamp INTEGER NOT NULL);";

SQLiteDeviceRepository::SQLiteDeviceRepository(const std::string& connectionString) : m_db(nullptr)
{
    // Attempt to open up a connection.
    auto rc = sqlite3_open(connectionString.c_str(), &m_db);
    if (rc != 0)
    {
        LOG(ERROR) << "Failed to open a connection to the Device Repository '" << connectionString << "'.";
        sqlite3_close(m_db);
        m_db = nullptr;
        return;
    }
    LOG(DEBUG) << "Successfully opened up a connection to the Device Repository '" << connectionString << "'.";

    // Create all the tables
    auto errorMessage = executeSQLStatement(CREATE_DEVICE_TABLE);
    if (!errorMessage.empty())
        throw std::runtime_error("Failed to initialize necessary tables: '" + errorMessage + "'.");
}

SQLiteDeviceRepository::~SQLiteDeviceRepository()
{
    std::lock_guard<std::recursive_mutex> lock{m_mutex};

    if (m_db)
    {
        LOG(DEBUG) << "Closed the connection to the Device Repository.";
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

bool SQLiteDeviceRepository::save(const std::vector<StoredDeviceInformation>& devices)
{
    // Establish the error prefix, and check whether a database session exists
    const auto errorPrefix = "Failed to save devices in the database - ";
    if (m_db == nullptr)
    {
        LOG(ERROR) << errorPrefix << "The database connection is not established.";
        return false;
    }

    // If the device is already present, go to the update routine
    auto newDevices = std::vector<StoredDeviceInformation>{};
    auto oldDevices = std::vector<StoredDeviceInformation>{};
    for (const auto& device : devices)
    {
        if (containsDevice(device.getDeviceKey()))
            oldDevices.emplace_back(device);
        else
            newDevices.emplace_back(device);
    }

    // Update the old devices
    if (!oldDevices.empty())
        update(oldDevices);

    // Store the information about the device
    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    auto errorMessage = executeSQLStatement("BEGIN TRANSACTION;");
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to start the database transaction - '" << errorMessage << "'.";
        return false;
    }
    for (const auto& device : newDevices)
    {
        // Create the device
        errorMessage = executeSQLStatement("INSERT INTO Device(DeviceKey, BelongsTo, Timestamp) VALUES ('" +
                                           device.getDeviceKey() + "', '" + toString(device.getDeviceBelongsTo()) +
                                           "', " + std::to_string(device.getTimestamp().count()) + ");");
        if (!errorMessage.empty())
        {
            executeSQLStatement("ROLLBACK;");
            LOG(ERROR) << errorPrefix << "Failed to insert device info into the database - '" << errorMessage << "'.";
            return false;
        }
    }
    executeSQLStatement("COMMIT;");
    return true;
}

bool SQLiteDeviceRepository::remove(const std::vector<std::string>& deviceKeys)
{
    // Establish the error prefix, and check whether a database session exists
    const auto errorPrefix = "Failed to remove devices from the database - ";
    if (m_db == nullptr)
    {
        LOG(ERROR) << errorPrefix << "The database connection is not established.";
        return false;
    }

    // Check if the vector is empty
    if (deviceKeys.empty())
    {
        LOG(ERROR) << errorPrefix << "The keys vector is empty.";
        return false;
    }

    // Make the string for the array in sql query
    auto arrayString = std::string{"("};
    for (auto i = std::uint64_t{0}; i < deviceKeys.size(); ++i)
        arrayString += "'" + deviceKeys[i] + "'" + (i < deviceKeys.size() - 1 ? ", " : "");
    arrayString += ")";

    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    auto errorMessage = executeSQLStatement("DELETE FROM Device WHERE Device.DeviceKey IN " + arrayString + ";");
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to execute the query - '" << errorMessage << "'.";
        return false;
    }
    return true;
}

bool SQLiteDeviceRepository::removeAll()
{
    // Establish the error prefix, and check whether a database session exists
    const auto errorPrefix = "Failed to remove all devices from the database - ";
    if (m_db == nullptr)
    {
        LOG(ERROR) << errorPrefix << "The database connection is not established.";
        return false;
    }

    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    auto errorMessage = executeSQLStatement("DELETE FROM Device;");
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to execute the query - '" << errorMessage << "'.";
        return false;
    }
    return true;
}

bool SQLiteDeviceRepository::containsDevice(const std::string& deviceKey)
{
    // Establish the error prefix, and check whether a database session exists
    const auto errorPrefix = "Failed to obtain information whether information about device exists - ";
    if (m_db == nullptr)
    {
        LOG(ERROR) << errorPrefix << "The database connection is not established.";
        return false;
    }

    auto result = ColumnResult{};
    {
        std::lock_guard<std::recursive_mutex> lock{m_mutex};
        auto errorMessage =
          executeSQLStatement("SELECT DeviceKey FROM Device WHERE Device.DeviceKey = '" + deviceKey + "';", &result);
        if (!errorMessage.empty())
        {
            LOG(ERROR) << errorPrefix << "Failed to execute the query - '" << errorMessage << "'.";
            return false;
        }
    }
    return result.size() >= 2 && result[1].front() == deviceKey;
}

StoredDeviceInformation SQLiteDeviceRepository::get(const std::string& deviceKey)
{
    // Establish the error prefix, and check whether a database session exists
    const auto errorPrefix = "Failed to obtain information about a device - ";
    if (m_db == nullptr)
    {
        LOG(ERROR) << errorPrefix << "The database connection is not established.";
        return {};
    }

    auto result = ColumnResult{};
    {
        std::lock_guard<std::recursive_mutex> lock{m_mutex};
        auto errorMessage = executeSQLStatement(
          "SELECT DeviceKey, BelongsTo, Timestamp FROM Device WHERE Device.DeviceKey = '" + deviceKey + "';", &result);
        if (!errorMessage.empty())
        {
            LOG(ERROR) << errorPrefix << "Failed to execute the query - '" << errorMessage << "'.";
            return {};
        }
    }
    if (result.size() < 2)
    {
        LOG(DEBUG) << errorPrefix << "Device not found in the database.";
        return {};
    }
    return loadDeviceInformationFromRow(result, 1);
}

std::vector<StoredDeviceInformation> SQLiteDeviceRepository::getGatewayDevices()
{
    // Establish the error prefix, and check whether a database session exists
    const auto errorPrefix = "Failed to obtain information about a device - ";
    if (m_db == nullptr)
    {
        LOG(ERROR) << errorPrefix << "The database connection is not established.";
        return {};
    }

    auto result = ColumnResult{};
    {
        std::lock_guard<std::recursive_mutex> lock{m_mutex};
        auto errorMessage = executeSQLStatement(
          "SELECT DeviceKey, BelongsTo, Timestamp FROM Device WHERE Device.BelongsTo = 'Gateway';", &result);
        if (!errorMessage.empty())
        {
            LOG(ERROR) << errorPrefix << "Failed to execute the query - '" << errorMessage << "'.";
            return {};
        }
    }

    auto devices = std::vector<StoredDeviceInformation>{};
    for (auto i = std::uint64_t{1}; i < result.size(); ++i)
    {
        auto device = loadDeviceInformationFromRow(result, i);
        if (device.getDeviceKey().empty())
            return {};
        devices.emplace_back(std::move(device));
    }
    return devices;
}

std::chrono::milliseconds SQLiteDeviceRepository::latestPlatformTimestamp()
{
    // Establish the error prefix, and check whether a database session exists
    const auto errorPrefix = "Failed to obtain the latest timestamp value - ";
    if (m_db == nullptr)
    {
        LOG(ERROR) << errorPrefix << "The database connection is not established.";
        return {};
    }

    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    auto result = ColumnResult{};
    auto millis = std::chrono::milliseconds{};
    auto errorMessage = executeSQLStatement("SELECT MAX(Timestamp) FROM Device;", &result);
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to execute the query - '" << errorMessage << "'.";
        return millis;
    }
    if (result.size() >= 2 && !result[1].empty())
    {
        try
        {
            millis = std::chrono::milliseconds{std::stoull(result[1].front())};
        }
        catch (const std::exception& exception)
        {
            LOG(ERROR) << errorPrefix << "Failed to convert string value into timestamp.";
            return millis;
        }
    }
    return millis;
}

bool SQLiteDeviceRepository::update(const std::vector<StoredDeviceInformation>& devices)
{
    // Establish the error prefix, and check whether a database session exists
    const auto errorPrefix = "Failed to update device information - ";

    // Store the information about the device
    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    auto errorMessage = executeSQLStatement("BEGIN TRANSACTION;");
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to start the database transaction - '" << errorMessage << "'.";
        return false;
    }
    for (const auto& device : devices)
    {
        // Create the device
        errorMessage = executeSQLStatement("UPDATE Device SET BelongsTo = '" + toString(device.getDeviceBelongsTo()) +
                                           "', Timestamp = " + std::to_string(device.getTimestamp().count()) +
                                           " WHERE DeviceKey = '" + device.getDeviceKey() + "' LIMIT 1;");
        if (!errorMessage.empty())
        {
            executeSQLStatement("ROLLBACK;");
            LOG(ERROR) << errorPrefix << "Failed to update device info in the database - '" << errorMessage << "'.";
            return false;
        }
    }
    executeSQLStatement("COMMIT;");
    return true;
}

StoredDeviceInformation SQLiteDeviceRepository::loadDeviceInformationFromRow(ColumnResult& result, std::uint64_t row)
{
    const auto errorPrefix = "Failed to load device information - ";

    const auto belongsTo = deviceOwnershipFromString(result[row][1]);
    if (belongsTo == DeviceOwnership::None)
    {
        LOG(ERROR) << errorPrefix << "Device contains invalid 'BelongsTo' value.";
        return {};
    }
    auto timestamp = std::chrono::milliseconds{};
    try
    {
        timestamp = std::chrono::milliseconds{std::stoull(result[row][2])};
    }
    catch (const std::exception&)
    {
        LOG(ERROR) << errorPrefix << "Device 'Timestamp' value could not be parsed.";
        return {};
    }
    return {result[row][0], belongsTo, timestamp};
}

std::string SQLiteDeviceRepository::executeSQLStatement(const std::string& sql, ColumnResult* result)
{
    const auto errorPrefix = "Failed to execute query - ";

    // Check if the database session is established
    if (m_db == nullptr)
    {
        auto errorMessage = "The database session is not established";
        LOG(ERROR) << errorPrefix << errorMessage;
        return errorMessage;
    }

    // If the query does not need to have a result, execute it
    if (result == nullptr)
    {
        auto errorMessage = std::string{};
        char* errorMessageCStr;
        auto rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errorMessageCStr);
        if (rc != SQLITE_OK)
        {
            LOG(ERROR) << errorPrefix << errorMessageCStr << "'.";
            errorMessage = errorMessageCStr;
            sqlite3_free(errorMessageCStr);
        }
        return errorMessage;
    }

    // Execute the query
    sqlite3_stmt* statement;
    auto rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &statement, nullptr);
    if (rc != SQLITE_OK)
    {
        auto errorMessage = std::string(sqlite3_errmsg(m_db));
        LOG(ERROR) << errorPrefix << errorMessage << "'.";
        return errorMessage;
    }

    // Go through the rows
    auto errorMessage = std::string{};
    auto entry = std::uint64_t{1};
    while (true)
    {
        // Check the next row
        rc = sqlite3_step(statement);
        if (rc == SQLITE_DONE)
            break;
        else if (rc != SQLITE_ROW)
        {
            errorMessage = sqlite3_errmsg(m_db);
            LOG(ERROR) << errorPrefix << errorMessage << "'.";
            break;
        }

        // Get the number of columns
        auto col = sqlite3_column_count(statement);

        // Check if the first result is empty
        if (result->find(0) == result->cend())
        {
            result->emplace(0, std::vector<std::string>{});
            for (auto i = std::int32_t{0}; i < col; ++i)
                (*result)[0].emplace_back(sqlite3_column_name(statement, i));
        }

        // Extract the data
        if (result->find(entry) == result->cend())
            result->emplace(entry, std::vector<std::string>{});
        for (auto i = std::int32_t{0}; i < col; ++i)
        {
            auto string = reinterpret_cast<const char*>(sqlite3_column_text(statement, i));
            if (string == nullptr)
                string = "";
            (*result)[entry].emplace_back(string);
        }
        ++entry;
    }
    sqlite3_finalize(statement);
    return errorMessage;
}
}    // namespace wolkabout::gateway
