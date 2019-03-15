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
#include "model/ActuatorTemplate.h"
#include "model/AlarmTemplate.h"
#include "model/ConfigurationTemplate.h"
#include "model/DataType.h"
#include "model/DetailedDevice.h"
#include "model/DeviceTemplate.h"

#include "utilities/Logger.h"

#include "Poco/Crypto/DigestEngine.h"
#include "Poco/Data/SQLite/Connector.h"
#include "Poco/Data/SQLite/SQLiteException.h"
#include "Poco/Data/Session.h"
#include "Poco/Data/Statement.h"
#include "Poco/String.h"
#include "Poco/Types.h"

#include <memory>
#include <mutex>
#include <regex>
#include <string>

namespace wolkabout
{
using namespace Poco::Data::Keywords;
using Poco::Data::Session;
using Poco::Data::Statement;

template <class T> WolkOptional<T> toOptional(const Poco::Nullable<T>& wrapper)
{
    if (wrapper.isNull())
        return WolkOptional<T>{};
    return WolkOptional<T>{wrapper.value()};
}

template <class T> Poco::Nullable<T> fromOptional(const WolkOptional<T>& wrapper)
{
    if (!wrapper)
        return Poco::Nullable<T>{};
    return Poco::Nullable<T>(wrapper.value());
}

SQLiteDeviceRepository::SQLiteDeviceRepository(const std::string& connectionString)
{
    Poco::Data::SQLite::Connector::registerConnector();
    m_session = std::unique_ptr<Session>(new Session(Poco::Data::SQLite::Connector::KEY, connectionString));

    Statement statement(*m_session);

    // Alarm template
    statement << "CREATE TABLE IF NOT EXISTS alarm_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, reference "
                 "TEXT, name TEXT, description TEXT, device_template_id INTEGER, "
                 "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    // Actuator template
    statement << "CREATE TABLE IF NOT EXISTS actuator_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                 "reference TEXT, name TEXT, description TEXT, unit_symbol TEXT, reading_type TEXT, "
                 "minimum REAL, maximum REAL, device_template_id INTEGER, "
                 "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    // Sensor template
    statement << "CREATE TABLE IF NOT EXISTS sensor_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                 "reference TEXT, name TEXT, description TEXT, unit_symbol TEXT, reading_type TEXT, "
                 "minimum REAL, maximum REAL, device_template_id INTEGER, "
                 "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    // Configuration template
    statement << "CREATE TABLE IF NOT EXISTS configuration_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                 "reference TEXT, name TEXT, description TEXT, "
                 "data_type TEXT, minimum REAL, maximum REAL, "
                 "default_value TEXT, device_template_id INTEGER, "
                 "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    statement
      << "CREATE TABLE IF NOT EXISTS configuration_label (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, label TEXT, "
         "configuration_template_id INTEGER, "
         "FOREIGN KEY(configuration_template_id) REFERENCES configuration_template(id) ON DELETE CASCADE);";

    // Device template
    statement << "CREATE TABLE IF NOT EXISTS device_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                 "firmware_update_protocol TEXT, sha256 TEXT);";

    // Type parameters
    statement << "CREATE TABLE IF NOT EXISTS type_parameters (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, key "
                 "TEXT, value TEXT, device_template_id INTEGER, "
                 "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    // Connectivity parameters
    statement
      << "CREATE TABLE IF NOT EXISTS connectivity_parameters (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, key "
         "TEXT, value TEXT, device_template_id INTEGER, "
         "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    // Firmware update parameters
    statement
      << "CREATE TABLE IF NOT EXISTS firmware_update_parameters (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, key "
         "TEXT, value TEXT, device_template_id INTEGER, "
         "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    // Device
    statement
      << "CREATE TABLE IF NOT EXISTS device (key TEXT PRIMARY KEY, name TEXT, device_template_id INTEGER NOT NULL, "
         "FOREIGN KEY(device_template_id) REFERENCES device_template(id));";

    statement << "PRAGMA foreign_keys=on;";

    statement.execute();
}

void SQLiteDeviceRepository::save(const DetailedDevice& device)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    try
    {
        Statement statement(*m_session);

        Poco::UInt64 devicesWithGivenKeyCount;
        statement << "SELECT count(*) FROM device WHERE device.key=?;", useRef(device.getKey()),
          into(devicesWithGivenKeyCount), now;

        if (devicesWithGivenKeyCount != 0)
        {
            update(device);
            return;
        }

        const std::string deviceTemplateSha256 = calculateSha256(device.getTemplate());
        statement.reset(*m_session);
        Poco::UInt64 matchingDeviceTemplatesCount;
        statement << "SELECT count(*) FROM device_template WHERE sha256=?;", useRef(deviceTemplateSha256),
          into(matchingDeviceTemplatesCount), now;
        if (matchingDeviceTemplatesCount != 0)
        {
            // Equivalent template exists
            statement.reset(*m_session);
            statement << "INSERT INTO device SELECT ?, ?, id FROM device_template WHERE device_template.sha256=?;",
              useRef(device.getKey()), useRef(device.getName()), useRef(deviceTemplateSha256), now;
            return;
        }

        // Create new device template
        statement.reset(*m_session);
        statement << "BEGIN TRANSACTION;";
        statement << "INSERT INTO device_template(firmware_update_protocol, sha256) VALUES(?, ?);",
          useRef(device.getTemplate().getFirmwareUpdateType()), useRef(deviceTemplateSha256);

        Poco::UInt64 deviceTemplateId;
        statement << "SELECT last_insert_rowid();", into(deviceTemplateId);

        // Alarm templates
        for (const wolkabout::AlarmTemplate& alarmTemplate : device.getTemplate().getAlarms())
        {
            statement << "INSERT INTO alarm_template(reference, name, description, device_template_id) "
                         "VALUES(?, ?, ?, ?);",
              useRef(alarmTemplate.getReference()), useRef(alarmTemplate.getName()),
              useRef(alarmTemplate.getDescription()), useRef(deviceTemplateId);
        }

        // Actuator templates
        for (const wolkabout::ActuatorTemplate& actuatorTemplate : device.getTemplate().getActuators())
        {
            Poco::Nullable<double> minimum = fromOptional(actuatorTemplate.getMinimum());
            Poco::Nullable<double> maximum = fromOptional(actuatorTemplate.getMaximum());

            statement << "INSERT INTO actuator_template(reference, name, description, unit_symbol, reading_type, "
                         "minimum, maximum, device_template_id) "
                         "VALUES(?, ?, ?, ?, ?, ?, ?, ?);",
              useRef(actuatorTemplate.getReference()), useRef(actuatorTemplate.getName()),
              useRef(actuatorTemplate.getDescription()), useRef(actuatorTemplate.getUnitSymbol()),
              useRef(actuatorTemplate.getReadingTypeName()), bind(minimum), bind(maximum), useRef(deviceTemplateId);
        }

        // Sensor templates
        for (const wolkabout::SensorTemplate& sensorTemplate : device.getTemplate().getSensors())
        {
            Poco::Nullable<double> minimum = fromOptional(sensorTemplate.getMinimum());
            Poco::Nullable<double> maximum = fromOptional(sensorTemplate.getMaximum());

            statement << "INSERT INTO sensor_template(reference, name, description, unit_symbol, reading_type, "
                         "minimum, maximum, device_template_id) "
                         "VALUES(?, ?, ?, ?, ?, ?, ?, ?);",
              useRef(sensorTemplate.getReference()), useRef(sensorTemplate.getName()),
              useRef(sensorTemplate.getDescription()), useRef(sensorTemplate.getUnitSymbol()),
              useRef(sensorTemplate.getReadingTypeName()), bind(minimum), bind(maximum), useRef(deviceTemplateId);
        }

        // Configuration templates
        for (const wolkabout::ConfigurationTemplate& configurationTemplate : device.getTemplate().getConfigurations())
        {
            const auto dataType = [&]() -> std::string {
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

            Poco::Nullable<double> minimum = fromOptional(configurationTemplate.getMinimum());
            Poco::Nullable<double> maximum = fromOptional(configurationTemplate.getMaximum());

            statement << "INSERT INTO configuration_template(reference, name, description, data_type, minimum, "
                         "maximum, default_value, device_template_id)"
                         "VALUES(?, ?, ?, ?, ?, ?, ?, ?);",
              useRef(configurationTemplate.getReference()), useRef(configurationTemplate.getName()),
              useRef(configurationTemplate.getDescription()), bind(dataType), bind(minimum), bind(maximum),
              useRef(configurationTemplate.getDefaultValue()), useRef(deviceTemplateId);

            for (const std::string& label : configurationTemplate.getLabels())
            {
                statement << "INSERT INTO configuration_label SELECT NULL, ?, id FROM configuration_template WHERE "
                             "configuration_template.reference=? AND configuration_template.device_template_id=?;",
                  useRef(label), useRef(configurationTemplate.getReference()), useRef(deviceTemplateId);
            }
        }

        // Type parameters
        for (auto const& parameter : device.getTemplate().getTypeParameters())
        {
            statement << "INSERT INTO type_parameters(key, value, device_template_id)"
                         "VALUES(?, ?, ?);",
              useRef(parameter.first), useRef(parameter.second), useRef(deviceTemplateId);
        }

        // Connectivity parameters
        for (auto const& parameter : device.getTemplate().getConnectivityParameters())
        {
            statement << "INSERT INTO connectivity_parameters(key, value, device_template_id)"
                         "VALUES(?, ?, ?);",
              useRef(parameter.first), useRef(parameter.second), useRef(deviceTemplateId);
        }

        // Firmware update parameters
        for (auto const& parameter : device.getTemplate().getFirmwareUpdateParameters())
        {
            statement << "INSERT INTO firmware_update_parameters(key, value, device_template_id)"
                         "VALUES(?, ?, ?);",
              useRef(parameter.first), useRef(parameter.second), useRef(deviceTemplateId);
        }

        // Device
        statement << "INSERT INTO device(key, name, device_template_id) VALUES(?, ?, ?);", useRef(device.getKey()),
          useRef(device.getName()), useRef(deviceTemplateId);
        statement << "COMMIT;", now;
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteDeviceRepository: Error saving device with key " << device.getKey();
    }
}

void SQLiteDeviceRepository::remove(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    try
    {
        Statement statement(*m_session);

        Poco::UInt64 deviceTemplateId;
        statement << "SELECT device_template_id FROM device WHERE device.key=?;", useRef(deviceKey),
          into(deviceTemplateId);
        if (statement.execute() == 0)
        {
            return;
        }

        statement.reset(*m_session);
        Poco::UInt64 numberOfDevicesReferencingTemplate;
        statement << "SELECT count(*) FROM device WHERE device_template_id=?;", useRef(deviceTemplateId),
          into(numberOfDevicesReferencingTemplate), now;
        if (numberOfDevicesReferencingTemplate != 1)
        {
            statement.reset(*m_session);
            statement << "DELETE FROM device WHERE device.key=?;", useRef(deviceKey), now;
            return;
        }

        statement.reset(*m_session);
        statement << "BEGIN TRANSACTION;";

        statement << "DELETE FROM device          WHERE device.key=?;", useRef(deviceKey);
        statement << "DELETE FROM device_template WHERE device_template.id=?;", useRef(deviceTemplateId);

        statement << "COMMIT;", now;
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteDeviceRepository: Error removing device with key " << deviceKey;
    }
}

void SQLiteDeviceRepository::removeAll()
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    const auto deviceKeysFromRepository = findAllDeviceKeys();
    for (const std::string& deviceKey : *deviceKeysFromRepository)
    {
        remove(deviceKey);
    }
}

std::unique_ptr<DetailedDevice> SQLiteDeviceRepository::findByDeviceKey(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    Poco::UInt64 deviceTemplateId;
    std::string deviceName;

    try
    {
        Statement statement(*m_session);
        statement << "SELECT name, device_template_id FROM device WHERE device.key=?;", useRef(deviceKey),
          into(deviceName), into(deviceTemplateId);
        if (statement.execute() == 0)
        {
            return nullptr;
        }
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteDeviceRepository: Error finding device with key " << deviceKey;
        return nullptr;
    }

    try
    {
        // Device template
        std::string firmwareUpdateProtocol;

        Statement statement(*m_session);
        statement << "SELECT firmware_update_protocol FROM device_template WHERE id=?;", useRef(deviceTemplateId),
          into(firmwareUpdateProtocol), now;

        auto deviceTemplate =
          std::unique_ptr<DeviceTemplate>(new DeviceTemplate({}, {}, {}, {}, firmwareUpdateProtocol));

        // Alarm templates
        std::string alarmReference;
        std::string alarmName;
        std::string alarmDescription;
        statement.reset(*m_session);
        statement << "SELECT reference, name, description FROM alarm_template WHERE device_template_id=?;",
          useRef(deviceTemplateId), into(alarmReference), into(alarmName), into(alarmDescription), range(0, 1);

        while (!statement.done())
        {
            if (statement.execute() == 0)
            {
                break;
            }

            deviceTemplate->addAlarm(AlarmTemplate(alarmName, alarmReference, alarmDescription));
        }

        // Actuator templates
        std::string actuatorReference;
        std::string actuatorName;
        std::string actuatorDescription;
        std::string actuatorUnitSymbol;
        std::string actuatorReadingType;
        Poco::Nullable<double> actuatorMinimum;
        Poco::Nullable<double> actuatorMaximum;
        statement.reset(*m_session);
        statement << "SELECT reference, name, description, unit_symbol, reading_type, "
                     "minimum, maximum "
                     "FROM actuator_template WHERE device_template_id=?;",
          useRef(deviceTemplateId), into(actuatorReference), into(actuatorName), into(actuatorDescription),
          into(actuatorUnitSymbol), into(actuatorReadingType), into(actuatorMinimum), into(actuatorMaximum),
          range(0, 1);

        while (!statement.done())
        {
            if (statement.execute() == 0)
            {
                break;
            }

            deviceTemplate->addActuator(ActuatorTemplate(actuatorName, actuatorReference, actuatorReadingType,
                                                         actuatorUnitSymbol, actuatorDescription,
                                                         toOptional(actuatorMinimum), toOptional(actuatorMaximum)));
        }

        // Sensor templates
        std::string sensorReference;
        std::string sensorName;
        std::string sensorDescription;
        std::string sensorUnitSymbol;
        std::string sensorReadingType;
        Poco::Nullable<double> sensorMinimum;
        Poco::Nullable<double> sensorMaximum;
        statement.reset(*m_session);
        statement << "SELECT reference, name, description, unit_symbol, reading_type, "
                     "minimum, maximum "
                     "FROM sensor_template WHERE device_template_id=?;",
          useRef(deviceTemplateId), into(sensorReference), into(sensorName), into(sensorDescription),
          into(sensorUnitSymbol), into(sensorReadingType), into(sensorMinimum), into(sensorMaximum), range(0, 1);

        while (!statement.done())
        {
            if (statement.execute() == 0)
            {
                break;
            }

            deviceTemplate->addSensor(SensorTemplate(sensorName, sensorReference, sensorReadingType, sensorUnitSymbol,
                                                     sensorDescription, toOptional(sensorMinimum),
                                                     toOptional(sensorMaximum)));
        }

        // Configuration templates
        Poco::UInt64 configurationTemplateId;
        std::string configurationReference;
        std::string configurationName;
        std::string configurationDescription;
        std::string configurationDataTypeStr;
        Poco::Nullable<double> configurationMinimum;
        Poco::Nullable<double> configurationMaximum;
        std::string configurationDefaultValue;
        statement.reset(*m_session);
        statement << "SELECT id, reference, name, description, data_type, minimum, maximum, default_value"
                     " FROM configuration_template WHERE device_template_id=?;",
          useRef(deviceTemplateId), into(configurationTemplateId), into(configurationReference),
          into(configurationName), into(configurationDescription), into(configurationDataTypeStr),
          into(configurationMinimum), into(configurationMaximum), into(configurationDefaultValue), range(0, 1);

        while (!statement.done())
        {
            if (statement.execute() == 0)
            {
                break;
            }

            const auto configurationDataType = [&]() -> DataType {
                if (configurationDataTypeStr == "STRING")
                {
                    return DataType::STRING;
                }
                else if (configurationDataTypeStr == "BOOLEAN")
                {
                    return DataType::BOOLEAN;
                }
                else if (configurationDataTypeStr == "NUMERIC")
                {
                    return DataType::NUMERIC;
                }

                return DataType::STRING;
            }();

            std::vector<std::string> labels;
            Statement selectLabelsStatement(*m_session);
            selectLabelsStatement << "SELECT label FROM configuration_label WHERE configuration_template_id=?;",
              bind(configurationTemplateId), into(labels), now;

            deviceTemplate->addConfiguration(ConfigurationTemplate(
              configurationName, configurationReference, configurationDataType, configurationDescription,
              configurationDefaultValue, labels, toOptional(configurationMinimum), toOptional(configurationMaximum)));
        }

        // Type parameters

        std::string typeParameterKey;
        std::string typeParameterValue;
        statement.reset(*m_session);
        statement << "SELECT key, value FROM type_parameters WHERE device_template_id=?;", useRef(deviceTemplateId),
          into(typeParameterKey), into(typeParameterValue), range(0, 1);

        while (!statement.done())
        {
            if (statement.execute() == 0)
            {
                break;
            }

            deviceTemplate->addTypeParameter(std::pair<std::string, std::string>(typeParameterKey, typeParameterValue));
        }

        // Connectivity parameters

        std::string connectivityParameterKey;
        std::string connectivityParameterValue;
        statement.reset(*m_session);
        statement << "SELECT key, value FROM connectivity_parameters WHERE device_template_id=?;",
          useRef(deviceTemplateId), into(connectivityParameterKey), into(connectivityParameterValue), range(0, 1);

        while (!statement.done())
        {
            if (statement.execute() == 0)
            {
                break;
            }

            deviceTemplate->addConnectivityParameter(
              std::pair<std::string, std::string>(connectivityParameterKey, connectivityParameterValue));
        }

        // Firmware update parameters

        std::string firmwareUpdateParameterKey;
        std::string firmwareUpdateParameterValue;
        statement.reset(*m_session);
        statement << "SELECT key, value FROM firmware_update_parameters WHERE device_template_id=?;",
          useRef(deviceTemplateId), into(firmwareUpdateParameterKey), into(firmwareUpdateParameterValue), range(0, 1);

        while (!statement.done())
        {
            if (statement.execute() == 0)
            {
                break;
            }

            bool firmwareUpdateParameterValueBool = (firmwareUpdateParameterValue == "true") ? true : false;
            std::pair<std::string, bool> firmwareUpdateParameterPair;
            firmwareUpdateParameterPair = std::make_pair(firmwareUpdateParameterKey, firmwareUpdateParameterValueBool);
            deviceTemplate->addFirmwareUpdateParameter(firmwareUpdateParameterPair);
        }

        return std::unique_ptr<DetailedDevice>(new DetailedDevice(deviceName, deviceKey, *deviceTemplate));
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteDeviceRepository: Error deserializing device with key " << deviceKey;
        return nullptr;
    }
}

std::unique_ptr<std::vector<std::string>> SQLiteDeviceRepository::findAllDeviceKeys()
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    auto deviceKeys = std::unique_ptr<std::vector<std::string>>(new std::vector<std::string>());

    try
    {
        Statement statement(*m_session);
        statement << "SELECT key FROM device;", into(*deviceKeys), now;
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteDeviceRepository: Error finding device keys";
    }

    return deviceKeys;
}

bool SQLiteDeviceRepository::containsDeviceWithKey(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    try
    {
        Poco::UInt64 deviceCount;
        Statement statement(*m_session);
        statement << "SELECT count(*) FROM device WHERE device.key=?;", useRef(deviceKey), into(deviceCount), now;
        return deviceCount != 0 ? true : false;
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteDeviceRepository: Error finding device with key " << deviceKey;
        return false;
    }
}

void SQLiteDeviceRepository::update(const DetailedDevice& device)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    remove(device.getKey());
    save(device);
}

std::string SQLiteDeviceRepository::calculateSha256(const AlarmTemplate& alarmTemplate)
{
    Poco::Crypto::DigestEngine digestEngine("SHA256");
    digestEngine.update(alarmTemplate.getName());
    digestEngine.update(alarmTemplate.getReference());
    digestEngine.update(alarmTemplate.getDescription());

    return Poco::Crypto::DigestEngine::digestToHex(digestEngine.digest());
}

std::string SQLiteDeviceRepository::calculateSha256(const ActuatorTemplate& actuatorTemplate)
{
    Poco::Crypto::DigestEngine digestEngine("SHA256");
    digestEngine.update(actuatorTemplate.getName());
    digestEngine.update(actuatorTemplate.getReference());
    digestEngine.update(actuatorTemplate.getDescription());
    digestEngine.update(actuatorTemplate.getUnitSymbol());
    digestEngine.update(actuatorTemplate.getReadingTypeName());
    digestEngine.update(std::to_string(actuatorTemplate.getMinimum().value()));
    digestEngine.update(std::to_string(actuatorTemplate.getMaximum().value()));

    return Poco::Crypto::DigestEngine::digestToHex(digestEngine.digest());
}

std::string SQLiteDeviceRepository::calculateSha256(const SensorTemplate& sensorTemplate)
{
    Poco::Crypto::DigestEngine digestEngine("SHA256");
    digestEngine.update(sensorTemplate.getName());
    digestEngine.update(sensorTemplate.getReference());
    digestEngine.update(sensorTemplate.getDescription());
    digestEngine.update(sensorTemplate.getUnitSymbol());
    digestEngine.update(sensorTemplate.getReadingTypeName());
    digestEngine.update(std::to_string(sensorTemplate.getMinimum().value()));
    digestEngine.update(std::to_string(sensorTemplate.getMaximum().value()));

    return Poco::Crypto::DigestEngine::digestToHex(digestEngine.digest());
}

std::string SQLiteDeviceRepository::calculateSha256(const ConfigurationTemplate& configurationTemplate)
{
    Poco::Crypto::DigestEngine digestEngine("SHA256");
    digestEngine.update(configurationTemplate.getName());
    digestEngine.update(configurationTemplate.getReference());
    digestEngine.update(configurationTemplate.getDescription());
    digestEngine.update(std::to_string(configurationTemplate.getMinimum().value()));
    digestEngine.update(std::to_string(configurationTemplate.getMaximum().value()));
    digestEngine.update(configurationTemplate.getDefaultValue());

    digestEngine.update([&]() -> std::string {
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

        poco_assert(false);
        return "";
    }());

    for (const std::string& label : configurationTemplate.getLabels())
    {
        digestEngine.update(label);
    }

    return Poco::Crypto::DigestEngine::digestToHex(digestEngine.digest());
}

std::string SQLiteDeviceRepository::calculateSha256(const std::pair<std::string, std::string>& typeParameter)
{
    Poco::Crypto::DigestEngine digestEngine("SHA256");
    digestEngine.update(typeParameter.first);
    digestEngine.update(typeParameter.second);

    return Poco::Crypto::DigestEngine::digestToHex(digestEngine.digest());
}

std::string SQLiteDeviceRepository::calculateSha256(const std::pair<std::string, bool>& firmwareUpdateParameter)
{
    Poco::Crypto::DigestEngine digestEngine("SHA256");
    digestEngine.update(firmwareUpdateParameter.first);
    bool firmwareUpdateParameterValueBool = (firmwareUpdateParameter.second == true) ? "true" : "false";
    digestEngine.update(firmwareUpdateParameterValueBool);

    return Poco::Crypto::DigestEngine::digestToHex(digestEngine.digest());
}

std::string SQLiteDeviceRepository::calculateSha256(const DeviceTemplate& deviceTemplate)
{
    Poco::Crypto::DigestEngine digestEngine("SHA256");
    digestEngine.update(deviceTemplate.getFirmwareUpdateType());

    for (const wolkabout::AlarmTemplate& alarmTemplate : deviceTemplate.getAlarms())
    {
        digestEngine.update(calculateSha256(alarmTemplate));
    }

    for (const wolkabout::ActuatorTemplate& actuatorTemplate : deviceTemplate.getActuators())
    {
        digestEngine.update(calculateSha256(actuatorTemplate));
    }

    for (const wolkabout::SensorTemplate& sensorTemplate : deviceTemplate.getSensors())
    {
        digestEngine.update(calculateSha256(sensorTemplate));
    }

    for (const wolkabout::ConfigurationTemplate& configurationTemplate : deviceTemplate.getConfigurations())
    {
        digestEngine.update(calculateSha256(configurationTemplate));
    }

    for (const std::pair<std::string, std::string>& typeParameter : deviceTemplate.getTypeParameters())
    {
        digestEngine.update(calculateSha256(typeParameter));
    }

    for (const std::pair<std::string, std::string>& connectivityParameter : deviceTemplate.getConnectivityParameters())
    {
        digestEngine.update(calculateSha256(connectivityParameter));
    }

    for (const std::pair<std::string, bool>& firmwareUpdateParameter : deviceTemplate.getFirmwareUpdateParameters())
    {
        digestEngine.update(calculateSha256(firmwareUpdateParameter));
    }

    return Poco::Crypto::DigestEngine::digestToHex(digestEngine.digest());
}

}    // namespace wolkabout
