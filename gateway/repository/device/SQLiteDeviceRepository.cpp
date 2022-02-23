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
#include "core/utilities/ByteUtils.h"
#include "core/utilities/Logger.h"

#include <memory>
#include <mutex>
#include <openssl/sha.h>
#include <sqlite3.h>
#include <string>

namespace wolkabout
{
namespace gateway
{
// Here are some create table instructions
const std::string CREATE_DEVICE_TABLE = "CREATE TABLE IF NOT EXISTS Device (ID INTEGER PRIMARY KEY AUTOINCREMENT, "
                                        "DeviceKey TEXT NOT NULL UNIQUE, ExternalId TEXT, Timestamp INTEGER NOT NULL);";

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

bool SQLiteDeviceRepository::save(std::chrono::milliseconds timestamp, const RegisteredDeviceInformation& device)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to save a device in the database - ";

    // If the device is already present, go to the update routine
    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    if (containsDeviceKey(device.deviceKey))
        return update(timestamp, device);

    // Store the information about the device
    auto errorMessage = executeSQLStatement("BEGIN TRANSACTION;");
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to start the database transaction - '" << errorMessage << "'.";
        return false;
    }

    // Create the device
    errorMessage =
      executeSQLStatement("INSERT INTO Device(DeviceKey, ExternalId, Timestamp) VALUES ('" + device.deviceKey + "', " +
                          (device.externalId.empty() ? "null" : "'" + device.externalId + "'") + ", " +
                          std::to_string(timestamp.count()) + ");");
    if (!errorMessage.empty())
    {
        executeSQLStatement("ROLLBACK;");
        LOG(ERROR) << errorPrefix << "Failed to insert device info into the database - '" << errorMessage << "'.";
        return false;
    }

    executeSQLStatement("COMMIT;");
    return true;
}

bool SQLiteDeviceRepository::remove(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to remove a device from the database - ";

    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    auto errorMessage = executeSQLStatement("DELETE FROM Device WHERE Device.DeviceKey = '" + deviceKey + "';");
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to execute the query - '" << errorMessage << "'.";
        return false;
    }
    return true;
}

bool SQLiteDeviceRepository::removeAll()
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to remove all devices from the database - ";

    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    auto errorMessage = executeSQLStatement("DELETE FROM Device;");
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to execute the query - '" << errorMessage << "'.";
        return false;
    }
    return true;
}

bool SQLiteDeviceRepository::containsDeviceKey(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to obtain information whether device info is stored - ";

    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    auto result = ColumnResult{};
    auto errorMessage =
      executeSQLStatement("SELECT DeviceKey FROM Device WHERE Device.DeviceKey = '" + deviceKey + "';", &result);
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to execute the query - '" << errorMessage << "'.";
        return false;
    }
    return result.size() >= 2 && result[1].front() == deviceKey;
}

std::chrono::milliseconds SQLiteDeviceRepository::latestTimestamp()
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to obtain the latest timestamp value - ";

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

bool SQLiteDeviceRepository::update(std::chrono::milliseconds timestamp, const RegisteredDeviceInformation& device)
{
    if (remove(device.deviceKey))
        return save(timestamp, device);
    else
        return false;
}

std::string SQLiteDeviceRepository::executeSQLStatement(const std::string& sql, ColumnResult* result)
{
    // This would spam too much
    //    LOG(TRACE) << METHOD_INFO;
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
}    // namespace gateway
}    // namespace wolkabout
