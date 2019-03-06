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

SQLiteDeviceRepository::SQLiteDeviceRepository(const std::string& connectionString)
{
    Poco::Data::SQLite::Connector::registerConnector();
    m_session = std::unique_ptr<Session>(new Session(Poco::Data::SQLite::Connector::KEY, connectionString));

    Statement statement(*m_session);

    // Alarm template
    statement << "CREATE TABLE IF NOT EXISTS alarm_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, reference "
                 "TEXT, name TEXT, severity TEXT, message TEXT, description TEXT, device_template_id INTEGER, "
                 "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    // Actuator template
    statement << "CREATE TABLE IF NOT EXISTS actuator_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                 "reference TEXT, name TEXT, description TEXT, unit_symbol TEXT, reading_type TEXT, data_type TEXT, "
                 "precision INTEGER, minimum REAL, maximum REAL, delimiter TEXT, device_template_id INTEGER, "
                 "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    statement << "CREATE TABLE IF NOT EXISTS actuator_label (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, label "
                 "TEXT, actuator_template_id INTEGER, "
                 "FOREIGN KEY(actuator_template_id) REFERENCES actuator_template(id) ON DELETE CASCADE);";

    // Sensor template
    statement << "CREATE TABLE IF NOT EXISTS sensor_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                 "reference TEXT, name TEXT, description TEXT, unit_symbol TEXT, reading_type TEXT, data_type TEXT, "
                 "precision INTEGER, minimum REAL, maximum REAL, delimiter TEXT, device_template_id INTEGER, "
                 "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    statement << "CREATE TABLE IF NOT EXISTS sensor_label (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, label TEXT, "
                 "sensor_template_id INTEGER, "
                 "FOREIGN KEY(sensor_template_id) REFERENCES sensor_template(id) ON DELETE CASCADE);";

    // Configuration template
    statement << "CREATE TABLE IF NOT EXISTS configuration_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                 "reference TEXT, name TEXT, description TEXT, "
                 "data_type TEXT, minimum REAL, maximum REAL, delimiter TEXT, "
                 "default_value TEXT, device_template_id INTEGER, "
                 "FOREIGN KEY(device_template_id) REFERENCES device_template(id) ON DELETE CASCADE);";

    statement
      << "CREATE TABLE IF NOT EXISTS configuration_label (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, label TEXT, "
         "configuration_template_id INTEGER, "
         "FOREIGN KEY(configuration_template_id) REFERENCES configuration_template(id) ON DELETE CASCADE);";

    // Device template
    statement << "CREATE TABLE IF NOT EXISTS device_template (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, name "
                 "TEXT, description TEXT, protocol TEXT, firmware_update_protocol TEXT, sha256 TEXT);";

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
        statement
          << "INSERT INTO device_template(name, description, protocol, firmware_update_protocol, sha256) VALUES(?, "
             "?, ?, ?, ?);",
          useRef(device.getTemplate().getName()), useRef(device.getTemplate().getDescription()),
          useRef(device.getTemplate().getProtocol()), useRef(device.getTemplate().getFirmwareUpdateType()),
          useRef(deviceTemplateSha256);

        Poco::UInt64 deviceTemplateId;
        statement << "SELECT last_insert_rowid();", into(deviceTemplateId);

        // Alarm templates
        for (const wolkabout::AlarmTemplate& alarmTemplate : device.getTemplate().getAlarms())
        {
            const auto severity = [&]() -> std::string {
                if (alarmTemplate.getSeverity() == AlarmTemplate::AlarmSeverity::ALERT)
                {
                    return "ALERT";
                }
                else if (alarmTemplate.getSeverity() == AlarmTemplate::AlarmSeverity::CRITICAL)
                {
                    return "CRITICAL";
                }
                else if (alarmTemplate.getSeverity() == AlarmTemplate::AlarmSeverity::ERROR)
                {
                    return "ERROR";
                }

                return "";
            }();

            statement
              << "INSERT INTO alarm_template(reference, name, severity, message, description, device_template_id) "
                 "VALUES(?, ?, ?, ?, ?, ?);",
              useRef(alarmTemplate.getReference()), useRef(alarmTemplate.getName()), bind(severity),
              useRef(alarmTemplate.getMessage()), useRef(alarmTemplate.getDescription()), useRef(deviceTemplateId);
        }

        // Actuator templates
        for (const wolkabout::ActuatorTemplate& actuatorTemplate : device.getTemplate().getActuators())
        {
            const auto dataType = [&]() -> std::string {
                if (actuatorTemplate.getDataType() == DataType::BOOLEAN)
                {
                    return "BOOLEAN";
                }
                else if (actuatorTemplate.getDataType() == DataType::NUMERIC)
                {
                    return "NUMERIC";
                }
                else if (actuatorTemplate.getDataType() == DataType::STRING)
                {
                    return "STRING";
                }

                return "";
            }();

            const auto precision = Poco::UInt32(actuatorTemplate.getPrecision());
            const auto minimum = actuatorTemplate.getMinimum();
            const auto maximum = actuatorTemplate.getMaximum();
            statement
              << "INSERT INTO actuator_template(reference, name, description, unit_symbol, reading_type, data_type, "
                 "precision, minimum, maximum, delimiter, device_template_id) "
                 "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
              useRef(actuatorTemplate.getReference()), useRef(actuatorTemplate.getName()),
              useRef(actuatorTemplate.getDescription()), useRef(actuatorTemplate.getUnitSymbol()),
              useRef(actuatorTemplate.getReadingTypeName()), bind(dataType), bind(precision), bind(minimum),
              bind(maximum), useRef(actuatorTemplate.getDelimiter()), useRef(deviceTemplateId);

            for (const std::string& label : actuatorTemplate.getLabels())
            {
                statement << "INSERT INTO actuator_label SELECT NULL, ?, id FROM actuator_template WHERE "
                             "actuator_template.reference=? AND actuator_template.device_template_id=?;",
                  useRef(label), useRef(actuatorTemplate.getReference()), useRef(deviceTemplateId);
            }
        }

        // Sensor templates
        for (const wolkabout::SensorTemplate& sensorTemplate : device.getTemplate().getSensors())
        {
            const auto dataType = [&]() -> std::string {
                if (sensorTemplate.getDataType() == DataType::BOOLEAN)
                {
                    return "BOOLEAN";
                }
                else if (sensorTemplate.getDataType() == DataType::NUMERIC)
                {
                    return "NUMERIC";
                }
                else if (sensorTemplate.getDataType() == DataType::STRING)
                {
                    return "STRING";
                }

                return "";
            }();

            const auto precision = Poco::UInt32(sensorTemplate.getPrecision());
            const auto minimum = sensorTemplate.getMinimum();
            const auto maximum = sensorTemplate.getMaximum();
            statement
              << "INSERT INTO sensor_template(reference, name, description, unit_symbol, reading_type, data_type, "
                 "precision, minimum, maximum, delimiter, device_template_id) "
                 "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
              useRef(sensorTemplate.getReference()), useRef(sensorTemplate.getName()),
              useRef(sensorTemplate.getDescription()), useRef(sensorTemplate.getUnitSymbol()),
              useRef(sensorTemplate.getReadingTypeName()), bind(dataType), bind(precision), bind(minimum),
              bind(maximum), useRef(sensorTemplate.getDelimiter()), useRef(deviceTemplateId);

            for (const std::string& label : sensorTemplate.getLabels())
            {
                statement << "INSERT INTO sensor_label SELECT NULL, ?, id FROM sensor_template WHERE "
                             "sensor_template.reference=? AND sensor_template.device_template_id=?;",
                  useRef(label), useRef(sensorTemplate.getReference()), useRef(deviceTemplateId);
            }
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

            const auto minimum = configurationTemplate.getMinimum();
            const auto maximum = configurationTemplate.getMaximum();

            statement << "INSERT INTO configuration_template(reference, name, description, data_type, minimum, "
                         "maximum, delimiter, default_value, device_template_id)"
                         "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?);",
              useRef(configurationTemplate.getReference()), useRef(configurationTemplate.getName()),
              useRef(configurationTemplate.getDescription()), bind(dataType), bind(minimum), bind(maximum),
              useRef(configurationTemplate.getDelimiter()), useRef(configurationTemplate.getDefaultValue()),
              useRef(deviceTemplateId);

            for (const std::string& label : configurationTemplate.getLabels())
            {
                statement << "INSERT INTO configuration_label SELECT NULL, ?, id FROM configuration_template WHERE "
                             "configuration_template.reference=? AND configuration_template.device_template_id=?;",
                  useRef(label), useRef(configurationTemplate.getReference()), useRef(deviceTemplateId);
            }
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
        std::string templateName;
        std::string templateDescription;
        std::string protocol;
        std::string firmwareUpdateProtocol;

        Statement statement(*m_session);
        statement << "SELECT name, description, protocol, firmware_update_protocol FROM device_template WHERE id=?;",
          useRef(deviceTemplateId), into(templateName), into(templateDescription), into(protocol),
          into(firmwareUpdateProtocol), now;

        auto deviceTemplate = std::unique_ptr<DeviceTemplate>(
          new DeviceTemplate(templateName, templateDescription, protocol, firmwareUpdateProtocol));

        // Alarm templates
        std::string alarmReference;
        std::string alarmName;
        std::string alarmSeverityStr;
        std::string alarmMessage;
        std::string alarmDescription;
        statement.reset(*m_session);
        statement
          << "SELECT reference, name, severity, message, description FROM alarm_template WHERE device_template_id=?;",
          useRef(deviceTemplateId), into(alarmReference), into(alarmName), into(alarmSeverityStr), into(alarmMessage),
          into(alarmDescription), range(0, 1);

        while (!statement.done())
        {
            if (statement.execute() == 0)
            {
                break;
            }

            const auto alarmSeverity = [&]() -> AlarmTemplate::AlarmSeverity {
                if (alarmSeverityStr == "ALERT")
                {
                    return AlarmTemplate::AlarmSeverity::ALERT;
                }
                else if (alarmSeverityStr == "CRITICAL")
                {
                    return AlarmTemplate::AlarmSeverity::CRITICAL;
                }
                else if (alarmSeverityStr == "ERROR")
                {
                    return AlarmTemplate::AlarmSeverity::ERROR;
                }
                return AlarmTemplate::AlarmSeverity::ALERT;
            }();

            deviceTemplate->addAlarm(
              AlarmTemplate(alarmName, alarmSeverity, alarmReference, alarmMessage, alarmDescription));
        }

        // Actuator templates
        Poco::UInt64 actuatorTemplateId;
        std::string actuatorReference;
        std::string actuatorName;
        std::string actuatorDescription;
        std::string actuatorUnitSymbol;
        std::string actuatorReadingType;
        std::string actuatorDataTypeStr;
        Poco::UInt32 actuatorPrecision;
        double actuatorMinimum;
        double actuatorMaximum;
        std::string actuatorDelimiter;
        statement.reset(*m_session);
        statement << "SELECT id, reference, name, description, unit_symbol, reading_type, data_type, precision, "
                     "minimum, maximum, "
                     "delimiter "
                     "FROM actuator_template WHERE device_template_id=?;",
          useRef(deviceTemplateId), into(actuatorTemplateId), into(actuatorReference), into(actuatorName),
          into(actuatorDescription), into(actuatorUnitSymbol), into(actuatorReadingType), into(actuatorDataTypeStr),
          into(actuatorPrecision), into(actuatorMinimum), into(actuatorMaximum), into(actuatorDelimiter), range(0, 1);

        while (!statement.done())
        {
            if (statement.execute() == 0)
            {
                break;
            }

            const auto actuatorDataType = [&]() -> DataType {
                if (actuatorDataTypeStr == "BOOLEAN")
                {
                    return DataType::BOOLEAN;
                }
                else if (actuatorDataTypeStr == "NUMERIC")
                {
                    return DataType::NUMERIC;
                }
                else if (actuatorDataTypeStr == "STRING")
                {
                    return DataType::STRING;
                }

                return DataType::STRING;
            }();

            std::vector<std::string> labels;
            Statement selectLabelsStatement(*m_session);
            selectLabelsStatement << "SELECT label FROM actuator_label WHERE actuator_template_id=?;",
              bind(actuatorTemplateId), into(labels), now;

            deviceTemplate->addActuator(ActuatorTemplate(
              actuatorName, actuatorReference, actuatorReadingType, actuatorUnitSymbol, actuatorDataType,
              actuatorPrecision, actuatorDescription, labels, actuatorMinimum, actuatorMaximum));
        }

        // Sensor templates
        Poco::UInt64 sensorTemplateId;
        std::string sensorReference;
        std::string sensorName;
        std::string sensorDescription;
        std::string sensorUnitSymbol;
        std::string sensorReadingType;
        std::string sensorDataTypeStr;
        Poco::UInt32 sensorPrecision;
        double sensorMinimum;
        double sensorMaximum;
        std::string sensorDelimiter;
        statement.reset(*m_session);
        statement << "SELECT id, reference, name, description, unit_symbol, reading_type, data_type, precision, "
                     "minimum, maximum, "
                     "delimiter "
                     "FROM sensor_template WHERE device_template_id=?;",
          useRef(deviceTemplateId), into(sensorTemplateId), into(sensorReference), into(sensorName),
          into(sensorDescription), into(sensorUnitSymbol), into(sensorReadingType), into(sensorDataTypeStr),
          into(sensorPrecision), into(sensorMinimum), into(sensorMaximum), into(sensorDelimiter), range(0, 1);

        while (!statement.done())
        {
            if (statement.execute() == 0)
            {
                break;
            }

            const auto sensorDataType = [&]() -> DataType {
                if (sensorDataTypeStr == "BOOLEAN")
                {
                    return DataType::BOOLEAN;
                }
                else if (sensorDataTypeStr == "NUMERIC")
                {
                    return DataType::NUMERIC;
                }
                else if (sensorDataTypeStr == "STRING")
                {
                    return DataType::STRING;
                }

                return DataType::STRING;
            }();

            std::vector<std::string> labels;
            Statement selectLabelsStatement(*m_session);
            selectLabelsStatement << "SELECT label FROM sensor_label WHERE sensor_template_id=?;",
              bind(sensorTemplateId), into(labels), now;

            deviceTemplate->addSensor(SensorTemplate(sensorName, sensorReference, sensorReadingType, sensorUnitSymbol,
                                                     sensorDataType, sensorPrecision, sensorDescription, labels,
                                                     sensorMinimum, sensorMaximum));
        }

        // Configuration templates
        Poco::UInt64 configurationTemplateId;
        std::string configurationReference;
        std::string configurationName;
        std::string configurationDescription;
        std::string configurationDataTypeStr;
        double configurationMinimum;
        double configurationMaximum;
        std::string configurationDelimiter;
        std::string configurationDefaultValue;
        statement.reset(*m_session);
        statement << "SELECT id, reference, name, description, data_type, minimum, maximum, delimiter, "
                     "default_value"
                     " FROM configuration_template WHERE device_template_id=?;",
          useRef(deviceTemplateId), into(configurationTemplateId), into(configurationReference),
          into(configurationName), into(configurationDescription), into(configurationDataTypeStr),
          into(configurationMinimum), into(configurationMaximum), into(configurationDelimiter),
          into(configurationDefaultValue), range(0, 1);

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
              configurationDefaultValue, labels, configurationMinimum, configurationMaximum));
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
    digestEngine.update(alarmTemplate.getMessage());
    digestEngine.update(alarmTemplate.getDescription());
    digestEngine.update([&]() -> std::string {
        if (alarmTemplate.getSeverity() == wolkabout::AlarmTemplate::AlarmSeverity::ALERT)
        {
            return "A";
        }
        else if (alarmTemplate.getSeverity() == wolkabout::AlarmTemplate::AlarmSeverity::CRITICAL)
        {
            return "C";
        }
        else if (alarmTemplate.getSeverity() == wolkabout::AlarmTemplate::AlarmSeverity::ERROR)
        {
            return "E";
        }

        poco_assert(false);
        return "";
    }());

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
    digestEngine.update(std::to_string(actuatorTemplate.getPrecision()));
    digestEngine.update(std::to_string(actuatorTemplate.getMinimum()));
    digestEngine.update(std::to_string(actuatorTemplate.getMaximum()));
    digestEngine.update(actuatorTemplate.getDelimiter());

    digestEngine.update([&]() -> std::string {
        if (actuatorTemplate.getDataType() == wolkabout::DataType::BOOLEAN)
        {
            return "B";
        }
        else if (actuatorTemplate.getDataType() == wolkabout::DataType::NUMERIC)
        {
            return "N";
        }
        else if (actuatorTemplate.getDataType() == wolkabout::DataType::STRING)
        {
            return "S";
        }

        poco_assert(false);
        return "";
    }());

    for (const std::string& label : actuatorTemplate.getLabels())
    {
        digestEngine.update(label);
    }

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
    digestEngine.update(std::to_string(sensorTemplate.getPrecision()));
    digestEngine.update(std::to_string(sensorTemplate.getMinimum()));
    digestEngine.update(std::to_string(sensorTemplate.getMaximum()));
    digestEngine.update(sensorTemplate.getDelimiter());

    digestEngine.update([&]() -> std::string {
        if (sensorTemplate.getDataType() == wolkabout::DataType::BOOLEAN)
        {
            return "B";
        }
        else if (sensorTemplate.getDataType() == wolkabout::DataType::NUMERIC)
        {
            return "N";
        }
        else if (sensorTemplate.getDataType() == wolkabout::DataType::STRING)
        {
            return "S";
        }

        poco_assert(false);
        return "";
    }());

    for (const std::string& label : sensorTemplate.getLabels())
    {
        digestEngine.update(label);
    }

    return Poco::Crypto::DigestEngine::digestToHex(digestEngine.digest());
}

std::string SQLiteDeviceRepository::calculateSha256(const ConfigurationTemplate& configurationTemplate)
{
    Poco::Crypto::DigestEngine digestEngine("SHA256");
    digestEngine.update(configurationTemplate.getName());
    digestEngine.update(configurationTemplate.getReference());
    digestEngine.update(configurationTemplate.getDescription());
    digestEngine.update(std::to_string(configurationTemplate.getMinimum()));
    digestEngine.update(std::to_string(configurationTemplate.getMaximum()));
    digestEngine.update(configurationTemplate.getDelimiter());
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

std::string SQLiteDeviceRepository::calculateSha256(const DeviceTemplate& deviceTemplate)
{
    Poco::Crypto::DigestEngine digestEngine("SHA256");
    digestEngine.update(deviceTemplate.getName());
    digestEngine.update(deviceTemplate.getDescription());
    digestEngine.update(deviceTemplate.getProtocol());
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

    return Poco::Crypto::DigestEngine::digestToHex(digestEngine.digest());
}

}    // namespace wolkabout
