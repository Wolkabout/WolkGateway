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

#include "SQLiteDeviceRepository.h"
#include "core/model/Device.h"
#include "core/utilities/ByteUtils.h"
#include "core/utilities/Logger.h"

#include <iomanip>
#include <iostream>
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
const std::string CREATE_DEVICE_TABLE =
  "CREATE TABLE IF NOT EXISTS Device (ID INTEGER PRIMARY KEY AUTOINCREMENT, DeviceName TEXT, DeviceKey TEXT NOT NULL "
  "UNIQUE, DevicePassword TEXT, DeviceDataMode TEXT NOT NULL CHECK(DeviceDataMode IN ('PUSH', 'PULL')));";
const std::string CREATE_FEED_TABLE =
  "CREATE TABLE IF NOT EXISTS Feed (ID INTEGER PRIMARY KEY AUTOINCREMENT, FeedName TEXT NOT NULL, FeedReference TEXT "
  "NOT NULL, FeedType TEXT NOT NULL CHECK(FeedType IN ('IN', 'IN_OUT')), FeedUnit TEXT NOT NULL, DeviceID INTEGER, "
  "FOREIGN KEY(DeviceID) REFERENCES Device(ID));";
const std::string CREATE_PARAMETER_TABLE =
  "CREATE TABLE IF NOT EXISTS Parameter (ID INTEGER PRIMARY KEY AUTOINCREMENT, ParameterName TEXT NOT NULL, "
  "ParameterValue TEXT, DeviceID INTEGER, FOREIGN KEY(DeviceId) REFERENCES Device(ID));";
const std::string CREATE_ATTRIBUTE_TABLE =
  "CREATE TABLE IF NOT EXISTS Attribute(ID INTEGER PRIMARY KEY AUTOINCREMENT, AttributeName TEXT NOT NULL, "
  "AttributeValue TEXT, DeviceID INTEGER, FOREIGN KEY(DeviceID) REFERENCES Device(ID));";
const std::string PRAGMA = "PRAGMA foreign_keys=on;";

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
    auto errorMessage = executeSQLStatement(CREATE_DEVICE_TABLE + CREATE_FEED_TABLE + CREATE_PARAMETER_TABLE +
                                            CREATE_ATTRIBUTE_TABLE + PRAGMA);
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

bool SQLiteDeviceRepository::save(const Device& device)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to save a device in the database - ";

    // If the device is already present, go to the update routine
    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    if (containsDeviceWithKey(device.getKey()))
        return update(device);

    // Store the information about the device
    auto errorMessage = executeSQLStatement("BEGIN TRANSACTION;");
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to start the database transaction - '" << errorMessage << "'.";
        return false;
    }

    // Create the device
    errorMessage =
      executeSQLStatement("INSERT INTO Device(DeviceName, DeviceKey, DevicePassword, DeviceDataMode) VALUES ('" +
                          device.getName() + "', '" + device.getKey() + "', '" + device.getPassword() + "', '" +
                          (device.getType() == OutboundDataMode::PUSH ? "PUSH" : "PULL") + "') ");
    if (!errorMessage.empty())
    {
        executeSQLStatement("ROLLBACK;");
        LOG(ERROR) << errorPrefix << "Failed to insert device info into the database - '" << errorMessage << "'.";
        return false;
    }

    // Get the row id
    std::uint64_t deviceId;
    {
        auto result = ColumnResult{};
        errorMessage = executeSQLStatement("SELECT last_insert_rowid();", &result);
        if (!errorMessage.empty())
        {
            executeSQLStatement("ROLLBACK;");
            LOG(ERROR) << errorPrefix << "Failed to obtain the id of the inserted device - '" << errorMessage << "'.";
            return false;
        }
        deviceId = std::stoul(result[1].front());
    }

    // Insert all the feeds
    for (const auto& feed : device.getFeeds())
    {
        errorMessage =
          executeSQLStatement("INSERT INTO Feed(FeedName, FeedReference, FeedType, FeedUnit, DeviceID) VALUES ('" +
                              feed.getName() + "', '" + feed.getReference() + "', '" + toString(feed.getFeedType()) +
                              "', '" + feed.getUnit() + "', " + std::to_string(deviceId) + ",);");
        if (!errorMessage.empty())
        {
            executeSQLStatement("ROLLBACK;");
            LOG(ERROR) << errorPrefix << "Failed to insert feed information for a device - '" << errorMessage << "'.";
            return false;
        }
    }

    // Insert all the parameters
    for (const auto& parameter : device.getParameters())
    {
        errorMessage = executeSQLStatement("INSERT INTO Parameter(ParameterName, ParameterValue, DeviceID) VALUES ('" +
                                           toString(parameter.first) + "', '" + parameter.second + "', " +
                                           std::to_string(deviceId) + ");");
        if (!errorMessage.empty())
        {
            executeSQLStatement("ROLLBACK;");
            LOG(ERROR) << errorPrefix << "Failed to insert parameter information for a device - '" << errorMessage
                       << "'.";
            return false;
        }
    }

    // Insert all the attributes
    for (const auto& attribute : device.getAttributes())
    {
        errorMessage = executeSQLStatement("INSERT INTO Attributes(AttributeName, AttributeValue, DeviceID) VALUES ('" +
                                           attribute.getName() + "', '" + attribute.getValue() + "', " +
                                           std::to_string(deviceId) + ")");
        if (!errorMessage.empty())
        {
            executeSQLStatement("ROLLBACK;");
            LOG(ERROR) << errorPrefix << "Failed to insert attribute information for a device - '" << errorMessage
                       << "'.";
            return false;
        }
    }

    executeSQLStatement("COMMIT;");
    return true;
}

bool SQLiteDeviceRepository::remove(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to remove a device from the database - ";

    std::lock_guard<std::recursive_mutex> lock{m_mutex};

    // See if there is a device with the device key in the database
    auto result = ColumnResult{};
    auto errorMessage =
      executeSQLStatement("SELECT ID FROM Device WHERE Device.DeviceKey = '" + deviceKey + "';", &result);
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to execute the query - '" << errorMessage << "'.";
        return false;
    }
    if (result.empty())
    {
        LOG(ERROR) << errorPrefix << "Device '" << deviceKey << "' is not in the database.";
        return false;
    }

    // Extract the device ID
    std::uint64_t deviceId;
    try
    {
        deviceId = std::stoull(result[1].front());
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << errorPrefix << "The device id obtained is not valid.";
        return false;
    }

    // Delete all the parts of the device
    executeSQLStatement("DELETE FROM Feed WHERE Feed.DeviceID = " + std::to_string(deviceId) + ";");
    executeSQLStatement("DELETE FROM Parameter WHERE Parameter.DeviceID = " + std::to_string(deviceId) + ";");
    executeSQLStatement("DELETE FROM Attribute WHERE Attribute.DeviceID = " + std::to_string(deviceId) + ";");

    // Delete this device
    executeSQLStatement("DELETE FROM Device WHERE Device.ID = " + std::to_string(deviceId) + ";");
    return true;
}

void SQLiteDeviceRepository::removeAll()
{
    LOG(TRACE) << METHOD_INFO;

    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    const auto deviceKeysFromRepository = findAllDeviceKeys();
    for (const std::string& deviceKey : *deviceKeysFromRepository)
        remove(deviceKey);
}

std::unique_ptr<Device> SQLiteDeviceRepository::findByDeviceKey(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to obtain device information from database - ";
    std::lock_guard<std::recursive_mutex> lock{m_mutex};

    // Try to find the device, and get its name and the template
    auto result = ColumnResult{};
    auto errorMessage = executeSQLStatement(
      "SELECT ID, DeviceName, DevicePassword, DeviceDataMode FROM Device WHERE Device.DeviceKey = '" + deviceKey + "';",
      &result);
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << errorMessage;
        return nullptr;
    }
    if (result.empty())
    {
        LOG(ERROR) << errorPrefix << "Found no information about device '" << deviceKey << "'.";
        return nullptr;
    }

    // Parse the received data about the device
    std::uint64_t deviceId;
    std::string deviceName;
    std::string devicePassword;
    OutboundDataMode deviceDataMode;
    try
    {
        // Go through the only row of data
        deviceId = std::stoull(result[1][0]);
        deviceName = result[1][1];
        devicePassword = result[1][2];
        if (result[1][3] == "PUSH")
            deviceDataMode = OutboundDataMode::PUSH;
        else if (result[1][3] == "PULL")
            deviceDataMode = OutboundDataMode::PULL;
        else
        {
            LOG(ERROR) << errorPrefix
                       << "Failed to parse received data about the device - The data mode value is not valid.";
            return nullptr;
        }
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << errorPrefix << "Failed to parse received data about the device - '" << exception.what() << "'.";
        return nullptr;
    }

    // Make the device
    auto device = std::unique_ptr<Device>{new Device{deviceKey, devicePassword, deviceDataMode, deviceName}};

    // Obtain the feeds
    errorMessage = executeSQLStatement(
      "SELECT FeedName, FeedReference, FeedType, FeedUnit FROM Feed WHERE Feed.DeviceID = " + std::to_string(deviceId) +
      ";");
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to obtain feed information '" << errorMessage << "'.";
        return nullptr;
    }

    // Parse them into a vector
    auto feeds = std::vector<Feed>{};
    for (auto i = std::uint64_t{1}; i < result.size(); ++i)
    {
        // Obtain the row
        const auto& feedRow = result[i];

        // Check that the feed type value is valid
        const auto feedType = feedTypeFromString(feedRow[2]);
        if (feedType == FeedType::NONE)
        {
            LOG(ERROR) << errorPrefix << "Feed information contains invalid data.";
            return nullptr;
        }

        // Emplace in the array
        feeds.emplace_back(feedRow[0], feedRow[1], feedType, feedRow[3]);
    }
    device->setFeeds(feeds);

    // Obtain the parameters
    errorMessage = executeSQLStatement(
      "SELECT ParameterName, ParameterValue FROM Parameter WHERE Parameter.DeviceID = " + std::to_string(deviceId) +
      ";");
    if (!errorMessage.empty())
    {
        LOG(ERROR) << errorPrefix << "Failed to obtain parameter information '" << errorMessage << "'.";
        return nullptr;
    }

    // Parse them into a map
    auto parameters = std::map<ParameterName, std::string>{};
    for (auto i = std::uint64_t{1}; i < result.size(); ++i)
    {
        // Obtain the row
        const auto& parameterRow = result[i];

        // Check that the parameter type value is valid
        const auto parameterName = parameterNameFromString(parameterRow[0]);
        if (parameterName == ParameterName::UNKNOWN)
        {
            LOG(ERROR) << errorPrefix << "Parameter information contains invalid data.";
            return nullptr;
        }

        // Emplace in the array
        parameters.emplace(parameterName, parameterRow[1]);
    }
    device->setParameters(parameters);

    // Return the device
    return device;
}

std::unique_ptr<std::vector<std::string>> SQLiteDeviceRepository::findAllDeviceKeys()
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::recursive_mutex> lock{m_mutex};

    // Make place for the query result
    auto result = ColumnResult{};
    executeSQLStatement("SELECT DeviceKey FROM Device;", &result);

    // Parse the results from the columns into the vector
    auto keys = std::unique_ptr<std::vector<std::string>>(new std::vector<std::string>);
    if (result.size() >= 2)
        for (auto i = std::uint64_t{1}; i < result.size(); ++i)
            keys->emplace_back(result[i].front());
    return keys;
}

bool SQLiteDeviceRepository::containsDeviceWithKey(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::recursive_mutex> lock{m_mutex};

    // Make place for the query result
    auto result = ColumnResult{};
    executeSQLStatement("SELECT count(*) FROM Device WHERE Device.DeviceKey = '" + deviceKey + "';", &result);
    if (result.empty())
    {
        LOG(ERROR) << "Failed to obtain the information if device is in the database.";
        return false;
    }
    return result[1][0] != "0";
}

bool SQLiteDeviceRepository::update(const Device& device)
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::recursive_mutex> lock{m_mutex};
    if (remove(device.getKey()))
        return save(device);
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
            (*result)[entry].emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(statement, i)));
        ++entry;
    }
    sqlite3_finalize(statement);
    return errorMessage;
}
}    // namespace gateway
}    // namespace wolkabout
