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

#include "repository/SQLiteDeviceRepository.h"

#include "core/model/ActuatorTemplate.h"
#include "core/model/AlarmTemplate.h"
#include "core/model/ConfigurationTemplate.h"
#include "core/model/DataType.h"
#include "core/model/DetailedDevice.h"
#include "core/model/DeviceTemplate.h"
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
// Here are some create table instructions
const std::string CREATE_ALARM_TABLE =
  "CREATE TABLE IF NOT EXISTS alarm_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, reference TEXT, name "
  "TEXT, description TEXT, device_template_id INTEGER, FOREIGN KEY(device_template_id) REFERENCES device_template(id) "
  "ON DELETE CASCADE);";
const std::string CREATE_ACTUATOR_TABLE =
  "CREATE TABLE IF NOT EXISTS actuator_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, reference TEXT, name "
  "TEXT, description TEXT, unit_symbol TEXT, reading_type TEXT, device_template_id INTEGER, FOREIGN "
  "KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";
const std::string CREATE_SENSOR_TABLE =
  "CREATE TABLE IF NOT EXISTS sensor_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, reference TEXT, name "
  "TEXT, description TEXT, unit_symbol TEXT, reading_type TEXT, device_template_id INTEGER, FOREIGN "
  "KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";
const std::string CREATE_CONFIGURATION_TABLE =
  "CREATE TABLE IF NOT EXISTS configuration_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, reference TEXT, "
  "name TEXT, description TEXT, data_type TEXT, default_value TEXT, device_template_id INTEGER, FOREIGN "
  "KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";
const std::string CREATE_CONFIGURATION_LABEL_TABLE =
  "CREATE TABLE IF NOT EXISTS configuration_label (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, label TEXT, "
  "configuration_template_id INTEGER, FOREIGN KEY(configuration_template_id) REFERENCES configuration_template(id) ON "
  "DELETE CASCADE);";
const std::string CREATE_DEVICE_TEMPLATE_TABLE =
  "CREATE TABLE IF NOT EXISTS device_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, firmware_update_protocol "
  "TEXT, sha256 TEXT);";
const std::string CREATE_TYPE_PARAMETERS_TABLE =
  "CREATE TABLE IF NOT EXISTS type_parameters (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, key TEXT, value TEXT, "
  "device_template_id INTEGER, FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";
const std::string CREATE_CONNECTIVITY_PARAMETERS_TABLE =
  "CREATE TABLE IF NOT EXISTS connectivity_parameters (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, key TEXT, value "
  "TEXT, device_template_id INTEGER, FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE "
  "CASCADE);";
const std::string CREATE_FIRMWARE_UPDATE_PARAMETERS_TABLE =
  "CREATE TABLE IF NOT EXISTS firmware_update_parameters (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, key TEXT, "
  "value INTEGER, device_template_id INTEGER, FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE "
  "CASCADE);";
const std::string CREATE_DEVICE_TABLE =
  "CREATE TABLE IF NOT EXISTS device (key TEXT PRIMARY KEY, name TEXT, device_template_id INTEGER NOT NULL, FOREIGN "
  "KEY(device_template_id) REFERENCES device_template(id));";
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
    executeSQLStatement(CREATE_ALARM_TABLE + CREATE_ACTUATOR_TABLE + CREATE_SENSOR_TABLE + CREATE_CONFIGURATION_TABLE +
                        CREATE_CONFIGURATION_LABEL_TABLE + CREATE_DEVICE_TEMPLATE_TABLE + CREATE_TYPE_PARAMETERS_TABLE +
                        CREATE_CONNECTIVITY_PARAMETERS_TABLE + CREATE_FIRMWARE_UPDATE_PARAMETERS_TABLE +
                        CREATE_DEVICE_TABLE + PRAGMA);
}

SQLiteDeviceRepository::~SQLiteDeviceRepository()
{
    if (m_db)
    {
        LOG(DEBUG) << "Closed the connection to the Device Repository.";
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

void SQLiteDeviceRepository::save(const DetailedDevice& device)
{
    LOG(TRACE) << METHOD_INFO;

    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    // Check if the device is already in the database
    auto result = ColumnResult{};
    executeSQLStatement("SELECT count(*) FROM device WHERE device.key = '" + device.getKey() + "';", &result);
    if (result[1][0] != "0")
    {
        update(device);
        return;
    }

    // Store the device template
    const auto& deviceTemplate = device.getTemplate();
    auto deviceTemplateHash = calculateSha256(deviceTemplate);
    result = {};
    executeSQLStatement("SELECT count(*) FROM device_template WHERE sha256 = '" + deviceTemplateHash + "';", &result);
    if (result[1][0] != "0")
    {
        // Equivalent template exists
        executeSQLStatement("INSERT INTO device SELECT '" + device.getKey() + "', '" + device.getName() +
                            "', id FROM device_template WHERE device_template.sha256 = '" + deviceTemplateHash + "';");
        return;
    }

    // Create new device template
    executeSQLStatement("BEGIN TRANSACTION;");
    executeSQLStatement("INSERT INTO device_template(firmware_update_protocol, sha256) VALUES('" +
                        deviceTemplate.getFirmwareUpdateType() + "', '" + deviceTemplateHash + "');");

    // Get the row id
    auto deviceTemplateId = std::uint64_t{0};
    try
    {
        result = {};
        executeSQLStatement("SELECT last_insert_rowid();", &result);
        deviceTemplateId = std::stoul(result[1].front());
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << "Failed to obtain the id of the newly inserted device template.";
        return;
    }

    // Insert all the alarms
    for (const auto& alarmTemplate : deviceTemplate.getAlarms())
    {
        executeSQLStatement("INSERT INTO alarm_template(reference, name, description, device_template_id) VALUES ('" +
                            alarmTemplate.getReference() + "', '" + alarmTemplate.getName() + "', '" +
                            alarmTemplate.getDescription() + "', " + std::to_string(deviceTemplateId) + ");");
    }

    // Insert all the actuators
    for (const auto& actuatorTemplate : deviceTemplate.getActuators())
    {
        executeSQLStatement("INSERT INTO actuator_template(reference, name, description, unit_symbol, reading_type, "
                            "device_template_id) VALUES('" +
                            actuatorTemplate.getReference() + "', '" + actuatorTemplate.getName() + "', '" +
                            actuatorTemplate.getDescription() + "', '" + actuatorTemplate.getUnitSymbol() + "', '" +
                            actuatorTemplate.getReadingTypeName() + "', " + std::to_string(deviceTemplateId) + ");");
    }

    // Insert all the sensors
    for (const auto& sensorTemplate : deviceTemplate.getSensors())
    {
        executeSQLStatement("INSERT INTO sensor_template(reference, name, description, unit_symbol, reading_type, "
                            "device_template_id) VALUES('" +
                            sensorTemplate.getReference() + "', '" + sensorTemplate.getName() + "', '" +
                            sensorTemplate.getDescription() + "', '" + sensorTemplate.getUnitSymbol() + "', '" +
                            sensorTemplate.getReadingTypeName() + "', " + std::to_string(deviceTemplateId) + ");");
    }

    // Insert all the configurations
    for (const auto& configurationTemplate : device.getTemplate().getConfigurations())
    {
        const auto dataTypeString = [&]() -> std::string {
            if (configurationTemplate.getDataType() == DataType::BOOLEAN)
            {
                return "BOOLEAN";
            }
            else if (configurationTemplate.getDataType() == DataType::NUMERIC)
            {
                return "NUMERIC";
            }
            else if (configurationTemplate.getDataType() == DataType::STRING)
            {
                return "STRING";
            }

            return "";
        }();

        executeSQLStatement("INSERT INTO configuration_template(reference, name, description, data_type, "
                            "default_value, device_template_id) VALUES('" +
                            configurationTemplate.getReference() + "', '" + configurationTemplate.getName() + "', '" +
                            configurationTemplate.getDescription() + "', '" + dataTypeString + "', '" +
                            configurationTemplate.getDefaultValue() + "', " + std::to_string(deviceTemplateId) + ");");

        for (const auto& label : configurationTemplate.getLabels())
        {
            executeSQLStatement(
              "INSERT INTO configuration_label SELECT NULL, '" + label +
              "', id FROM configuration_template WHERE "
              "configuration_template.reference = '" +
              configurationTemplate.getReference() +
              "' AND configuration_template.device_template_id = " + std::to_string(deviceTemplateId) + ";");
        }
    }

    // Insert all the type parameters
    for (const auto& parameter : deviceTemplate.getTypeParameters())
    {
        executeSQLStatement("INSERT INTO type_parameters(key, value, device_template_id) VALUES('" + parameter.first +
                            "', '" + parameter.second + "', " + std::to_string(deviceTemplateId) + ");");
    }

    // Insert all the firmware update parameters
    for (const auto& parameter : deviceTemplate.getFirmwareUpdateParameters())
    {
        executeSQLStatement("INSERT INTO firmware_update_parameters(key, value, device_template_id) VALUES('" +
                            parameter.first + "', " + std::to_string(parameter.second) + ", " +
                            std::to_string(deviceTemplateId) + ");");
    }

    for (const auto& parameter : deviceTemplate.getConnectivityParameters())
    {
        executeSQLStatement("INSERT INTO connectivity_parameters(key, value, device_template_id) VALUES('" +
                            parameter.first + "', '" + parameter.second + "', " + std::to_string(deviceTemplateId) +
                            ");");
    }

    for (const auto& parameter : deviceTemplate.getFirmwareUpdateParameters())
    {
        executeSQLStatement("INSERT INTO firmware_update_parameters(key, value, device_template_id) VALUES('" +
                            parameter.first + "', " + std::to_string(parameter.second) + ", " +
                            std::to_string(deviceTemplateId) + ");");
    }

    executeSQLStatement("INSERT INTO device(key, name, device_template_id) VALUES('" + device.getKey() + "', '" +
                        device.getName() + "', " + std::to_string(deviceTemplateId) + ");");
    executeSQLStatement("COMMIT;");
}

void SQLiteDeviceRepository::remove(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;

    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    // See if there is a device with the device key in the database
    auto result = ColumnResult{};
    executeSQLStatement("SELECT device_template_id FROM device WHERE device.key = '" + deviceKey + "';", &result);
    if (result.empty())
    {
        LOG(WARN) << "Tried to remove device with key '" << deviceKey << "' but device is not in the database.";
        return;
    }
    auto templateId = std::uint64_t{0};
    try
    {
        templateId = std::stoul(result[1].front());
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << "Failed to convert received template id into an integer.";
        return;
    }

    // Check for devices referencing the template
    result = {};
    executeSQLStatement("SELECT count(*) FROM device WHERE device_template_id = " + std::to_string(templateId) + ";",
                        &result);
    if (result.empty())
    {
        LOG(ERROR) << "Failed to obtain count of devices using device template " << templateId << ".";
        return;
    }
    auto numberOfDevicesReferenceTemplate = std::uint64_t{0};
    try
    {
        numberOfDevicesReferenceTemplate = std::stoul(result[1][0]);
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << "Failed to convert received count of devices into an integer.";
        return;
    }

    // Delete this device
    executeSQLStatement("DELETE FROM device WHERE device.key = '" + deviceKey + "';");

    // And if that was the only device, delete the template too
    if (numberOfDevicesReferenceTemplate < 2)
    {
        executeSQLStatement("DELETE FROM device_template WHERE device_template.id = " + std::to_string(templateId) +
                            ";");
    }
}

void SQLiteDeviceRepository::removeAll()
{
    LOG(TRACE) << METHOD_INFO;

    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    const auto deviceKeysFromRepository = findAllDeviceKeys();
    for (const std::string& deviceKey : *deviceKeysFromRepository)
    {
        remove(deviceKey);
    }
}

std::unique_ptr<DetailedDevice> SQLiteDeviceRepository::findByDeviceKey(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;

    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    // Try to find the device, and get its name and the template
    auto result = ColumnResult{};
    executeSQLStatement("SELECT name, device_template_id FROM device WHERE device.key = '" + deviceKey + "';", &result);
    if (result.empty())
    {
        return nullptr;
    }

    // Parse the received data about the device
    auto deviceTemplateId = std::uint64_t{0};
    auto deviceName = std::string{};
    try
    {
        // Go through the only row of data
        deviceName = result[1][0];
        deviceTemplateId = std::stoul(result[1][1]);
    }
    catch (const std::exception& exception)
    {
        LOG(ERROR) << "Failed to parse received data about the device.";
        return nullptr;
    }

    // Obtain the template
    auto deviceTemplate = getDeviceTemplate(deviceTemplateId);
    if (deviceTemplate == nullptr)
    {
        LOG(ERROR) << "Failed to obtain the device template for the device.";
        return nullptr;
    }

    // Create the device and return it
    return std::unique_ptr<DetailedDevice>(new DetailedDevice{deviceName, deviceKey, *deviceTemplate});
}

std::unique_ptr<DeviceTemplate> SQLiteDeviceRepository::getDeviceTemplate(std::uint64_t deviceTemplateId)
{
    LOG(TRACE) << METHOD_INFO;

    // Obtain all the entities need to form the template

    // Firmware update type
    auto result = ColumnResult{};
    auto firmwareUpdateType = std::string{};
    executeSQLStatement(
      "SELECT firmware_update_protocol FROM device_template WHERE id = " + std::to_string(deviceTemplateId) + ";",
      &result);
    if (result.empty())
    {
        LOG(ERROR) << "Failed to DeviceTemplate info for id " << deviceTemplateId << ".";
        return nullptr;
    }
    firmwareUpdateType = result[1][0];

    // Alarms
    result = {};
    auto alarms = std::vector<AlarmTemplate>{};
    executeSQLStatement("SELECT reference, name, description FROM alarm_template WHERE device_template_id = " +
                          std::to_string(deviceTemplateId) + ";",
                        &result);
    for (auto i = std::uint64_t{1}; i < result.size(); ++i)
    {
        alarms.emplace_back(result[i][1], result[i][0], result[i][2]);
    }

    // Actuators
    result = {};
    auto actuators = std::vector<ActuatorTemplate>{};
    executeSQLStatement("SELECT reference, name, description, unit_symbol, reading_type FROM actuator_template WHERE "
                        "device_template_id = " +
                          std::to_string(deviceTemplateId) + ";",
                        &result);
    for (auto i = std::uint64_t{1}; i < result.size(); ++i)
    {
        actuators.emplace_back(result[i][1], result[i][0], result[i][4], result[i][3], result[i][2]);
    }

    // Sensors
    result = {};
    auto sensors = std::vector<SensorTemplate>{};
    executeSQLStatement("SELECT reference, name, description, unit_symbol, reading_type FROM sensor_template WHERE "
                        "device_template_id = " +
                          std::to_string(deviceTemplateId) + ";",
                        &result);
    for (auto i = std::uint64_t{1}; i < result.size(); ++i)
    {
        sensors.emplace_back(result[i][1], result[i][0], result[i][4], result[i][3], result[i][2]);
    }

    // Configurations
    result = {};
    auto configurations = std::vector<ConfigurationTemplate>{};
    executeSQLStatement("SELECT id, reference, name, description, data_type, default_value FROM configuration_template "
                        "WHERE device_template_id = " +
                          std::to_string(deviceTemplateId) + ";",
                        &result);
    try
    {
        for (auto i = std::uint64_t{1}; i < result.size(); ++i)
        {
            configurations.emplace_back(
              result[i][2], result[i][1],
              [&]() -> DataType {
                  const auto dataType = result[i][4];
                  if (dataType == "STRING")
                      return DataType::STRING;
                  else if (dataType == "NUMERIC")
                      return DataType::NUMERIC;
                  else if (dataType == "BOOLEAN")
                      return DataType::BOOLEAN;
                  throw std::runtime_error("Failed to parse DataType.");
              }(),
              result[i][3], result[i][5]);
        }
    }
    catch (const std::exception& exception)
    {
        return nullptr;
    }

    // Now we can create the template
    auto deviceTemplate = std::unique_ptr<DeviceTemplate>{
      new DeviceTemplate{configurations, sensors, alarms, actuators, firmwareUpdateType}};

    // Type parameters
    result = {};
    executeSQLStatement(
      "SELECT key, value FROM type_parameters WHERE device_template_id = " + std::to_string(deviceTemplateId) + ";",
      &result);
    for (auto i = std::uint64_t{1}; i < result.size(); ++i)
    {
        deviceTemplate->addTypeParameter({result[i][0], result[i][1]});
    }

    // Connectivity parameters
    result = {};
    executeSQLStatement("SELECT key, value FROM connectivity_parameters WHERE device_template_id = " +
                          std::to_string(deviceTemplateId) + ";",
                        &result);
    for (auto i = std::uint64_t{1}; i < result.size(); ++i)
    {
        deviceTemplate->addConnectivityParameter({result[i][0], result[i][1]});
    }

    // Firmware update parameters
    result = {};
    executeSQLStatement("SELECT key, value FROM firmware_update_parameters WHERE device_template_id = " +
                          std::to_string(deviceTemplateId) + ";",
                        &result);
    for (auto i = std::uint64_t{1}; i < result.size(); ++i)
    {
        deviceTemplate->addFirmwareUpdateParameter({result[i][0], result[i][1] == "true"});
    }

    return deviceTemplate;
}

std::unique_ptr<std::vector<std::string>> SQLiteDeviceRepository::findAllDeviceKeys()
{
    LOG(TRACE) << METHOD_INFO;

    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    // Make place for the query result
    auto result = ColumnResult{};
    executeSQLStatement("SELECT key FROM device;", &result);

    // Parse the results from the columns into the vector
    auto keys = std::unique_ptr<std::vector<std::string>>(new std::vector<std::string>);
    if (result.size() >= 2)
    {
        for (auto i = std::uint64_t{1}; i < result.size(); ++i)
        {
            keys->emplace_back(result[i].front());
        }
    }
    return keys;
}

bool SQLiteDeviceRepository::containsDeviceWithKey(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;

    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    // Make place for the query result
    auto result = ColumnResult{};
    executeSQLStatement("SELECT count(*) FROM device WHERE device.key = '" + deviceKey + "';", &result);
    if (result.empty())
    {
        LOG(ERROR) << "Failed to obtain the information if device is in the database.";
        return false;
    }
    return result[1][0] != "0";
}

std::string SQLiteDeviceRepository::calculateSha256(const AlarmTemplate& alarmTemplate)
{
    // Make place for the hash
    std::uint8_t hash[SHA256_DIGEST_LENGTH];

    // Make the context, add the data
    auto ctx = SHA256_CTX{};
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, alarmTemplate.getName().c_str(), alarmTemplate.getName().length());
    SHA256_Update(&ctx, alarmTemplate.getReference().c_str(), alarmTemplate.getReference().length());
    SHA256_Update(&ctx, alarmTemplate.getDescription().c_str(), alarmTemplate.getDescription().length());
    SHA256_Final(hash, &ctx);

    // And parse it into a string
    return fromHashArray(hash);
}

std::string SQLiteDeviceRepository::calculateSha256(const ActuatorTemplate& actuatorTemplate)
{
    // Make place for the hash
    std::uint8_t hash[SHA256_DIGEST_LENGTH];

    // Make the context, add the data
    auto ctx = SHA256_CTX{};
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, actuatorTemplate.getName().c_str(), actuatorTemplate.getName().length());
    SHA256_Update(&ctx, actuatorTemplate.getReference().c_str(), actuatorTemplate.getReference().length());
    SHA256_Update(&ctx, actuatorTemplate.getDescription().c_str(), actuatorTemplate.getDescription().length());
    SHA256_Update(&ctx, actuatorTemplate.getUnitSymbol().c_str(), actuatorTemplate.getUnitSymbol().length());
    SHA256_Update(&ctx, actuatorTemplate.getReadingTypeName().c_str(), actuatorTemplate.getReadingTypeName().length());
    SHA256_Final(hash, &ctx);

    // And parse it all into a string
    return fromHashArray(hash);
}

std::string SQLiteDeviceRepository::calculateSha256(const SensorTemplate& sensorTemplate)
{
    // Make place for the hash
    std::uint8_t hash[SHA256_DIGEST_LENGTH];

    // Make the context, add the data
    auto ctx = SHA256_CTX{};
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, sensorTemplate.getName().c_str(), sensorTemplate.getName().length());
    SHA256_Update(&ctx, sensorTemplate.getReference().c_str(), sensorTemplate.getReference().length());
    SHA256_Update(&ctx, sensorTemplate.getDescription().c_str(), sensorTemplate.getDescription().length());
    SHA256_Update(&ctx, sensorTemplate.getUnitSymbol().c_str(), sensorTemplate.getUnitSymbol().length());
    SHA256_Update(&ctx, sensorTemplate.getReadingTypeName().c_str(), sensorTemplate.getReadingTypeName().length());
    SHA256_Final(hash, &ctx);

    // And parse it all into a string
    return fromHashArray(hash);
}

std::string SQLiteDeviceRepository::calculateSha256(const ConfigurationTemplate& configurationTemplate)
{
    // Make place for the hash
    std::uint8_t hash[SHA256_DIGEST_LENGTH];

    // Make the context, add the data
    auto ctx = SHA256_CTX{};
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, configurationTemplate.getName().c_str(), configurationTemplate.getName().length());
    SHA256_Update(&ctx, configurationTemplate.getReference().c_str(), configurationTemplate.getReference().length());
    SHA256_Update(&ctx, configurationTemplate.getDescription().c_str(),
                  configurationTemplate.getDescription().length());
    SHA256_Update(&ctx, configurationTemplate.getDefaultValue().c_str(),
                  configurationTemplate.getDefaultValue().length());
    auto dataTypeString = [&]() -> std::string {
        if (configurationTemplate.getDataType() == wolkabout::DataType::BOOLEAN)
        {
            return "B";
        }
        else if (configurationTemplate.getDataType() == wolkabout::DataType::NUMERIC)
        {
            return "N";
        }
        else if (configurationTemplate.getDataType() == wolkabout::DataType::STRING)
        {
            return "S";
        }
        return "";
    }();
    if (dataTypeString.empty())
        return "";
    SHA256_Update(&ctx, dataTypeString.c_str(), dataTypeString.length());
    for (const std::string& label : configurationTemplate.getLabels())
        SHA256_Update(&ctx, label.c_str(), label.length());
    SHA256_Final(hash, &ctx);

    // And parse it all into a string
    return fromHashArray(hash);
}

std::string SQLiteDeviceRepository::calculateSha256(const std::pair<std::string, std::string>& typeParameter)
{
    // Make place for the hash
    std::uint8_t hash[SHA256_DIGEST_LENGTH];

    // Make the context, add the data
    auto ctx = SHA256_CTX{};
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, typeParameter.first.c_str(), typeParameter.first.length());
    SHA256_Update(&ctx, typeParameter.second.c_str(), typeParameter.second.length());
    SHA256_Final(hash, &ctx);

    // And parse it into a string
    return fromHashArray(hash);
}

std::string SQLiteDeviceRepository::calculateSha256(const std::pair<std::string, bool>& firmwareUpdateParameter)
{
    // Make place for the hash
    std::uint8_t hash[SHA256_DIGEST_LENGTH];

    // Make the context, add the data
    auto ctx = SHA256_CTX{};
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, firmwareUpdateParameter.first.c_str(), firmwareUpdateParameter.first.length());
    auto secondString = std::string{firmwareUpdateParameter.second ? "true" : "false"};
    SHA256_Update(&ctx, secondString.c_str(), secondString.length());
    SHA256_Final(hash, &ctx);

    // And parse it into a string
    return fromHashArray(hash);
}

std::string SQLiteDeviceRepository::calculateSha256(const DeviceTemplate& deviceTemplate)
{
    // Make place for the hash
    std::uint8_t hash[SHA256_DIGEST_LENGTH];

    // Make the context, add the data
    auto ctx = SHA256_CTX{};
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, deviceTemplate.getFirmwareUpdateType().c_str(),
                  deviceTemplate.getFirmwareUpdateType().length());
    // Add the alarms
    for (const auto& alarmTemplate : deviceTemplate.getAlarms())
    {
        auto alarmHash = calculateSha256(alarmTemplate);
        SHA256_Update(&ctx, alarmHash.c_str(), alarmHash.length());
    }
    // Add the actuators
    for (const auto& actuatorTemplate : deviceTemplate.getActuators())
    {
        auto actuatorHash = calculateSha256(actuatorTemplate);
        SHA256_Update(&ctx, actuatorHash.c_str(), actuatorHash.length());
    }
    // Add the sensors
    for (const auto& sensorTemplate : deviceTemplate.getSensors())
    {
        auto sensorHash = calculateSha256(sensorTemplate);
        SHA256_Update(&ctx, sensorHash.c_str(), sensorHash.length());
    }
    // Add the configurations
    for (const auto& configurationTemplate : deviceTemplate.getConfigurations())
    {
        auto configurationHash = calculateSha256(configurationTemplate);
        SHA256_Update(&ctx, configurationHash.c_str(), configurationHash.length());
    }
    // Add the type parameters
    for (const auto& typeParameter : deviceTemplate.getTypeParameters())
    {
        auto typeHash = calculateSha256(typeParameter);
        SHA256_Update(&ctx, typeHash.c_str(), typeHash.length());
    }
    // Add the connectivity parameters
    for (const auto& connectivityParameter : deviceTemplate.getConnectivityParameters())
    {
        auto connectivityHash = calculateSha256(connectivityParameter);
        SHA256_Update(&ctx, connectivityHash.c_str(), connectivityHash.length());
    }
    // Add the firmware update parameters
    for (const auto& firmwareUpdateParameter : deviceTemplate.getFirmwareUpdateParameters())
    {
        auto firmwareUpdateHash = calculateSha256(firmwareUpdateParameter);
        SHA256_Update(&ctx, firmwareUpdateHash.c_str(), firmwareUpdateHash.length());
    }
    // And now finish the hash
    SHA256_Final(hash, &ctx);
    return fromHashArray(hash);
}

std::string SQLiteDeviceRepository::fromHashArray(std::uint8_t* array)
{
    // Make a string stream
    auto stream = std::stringstream{};

    // Go through the array
    for (auto i = std::uint32_t{0}; i < SHA256_DIGEST_LENGTH; ++i)
        stream << std::setw(2) << std::setfill('0') << std::hex << static_cast<std::int32_t>(array[i]);

    // Return the string
    return stream.str();
}

void SQLiteDeviceRepository::update(const DetailedDevice& device)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);
    remove(device.getKey());
    save(device);
}

void SQLiteDeviceRepository::executeSQLStatement(const std::string& sql, ColumnResult* result)
{
    // Check if the database session is established
    if (m_db == nullptr)
    {
        LOG(ERROR) << "Failed to execute query - The database session is not established.";
        return;
    }

    // If the query does not need to have a result, execute it
    if (result == nullptr)
    {
        char* errorMessage;
        auto rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errorMessage);
        if (rc != SQLITE_OK)
        {
            LOG(ERROR) << "Failed to execute query - '" << errorMessage << "'.";
            return;
        }
    }

    // Execute the query
    sqlite3_stmt* statement;
    auto rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &statement, nullptr);
    if (rc != SQLITE_OK)
    {
        LOG(ERROR) << "Failed to execute query - '" << sqlite3_errmsg(m_db) << "'.";
        return;
    }

    // Go through the rows
    auto entry = std::uint64_t{1};
    while (true)
    {
        // Check the next row
        rc = sqlite3_step(statement);
        if (rc == SQLITE_DONE)
        {
            return;
        }
        else if (rc != SQLITE_ROW)
        {
            LOG(ERROR) << "Failed to execute query - '" << sqlite3_errmsg(m_db) << "'.";
            return;
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
}
}    // namespace wolkabout
