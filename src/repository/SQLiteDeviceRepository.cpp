#include "repository/SQLiteDeviceRepository.h"
#include "model/ActuatorManifest.h"
#include "model/AlarmManifest.h"
#include "model/ConfigurationManifest.h"
#include "model/Device.h"
#include "model/DeviceManifest.h"

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

    // Alarm manifest
    statement << "CREATE TABLE IF NOT EXISTS alarm_manifest (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, reference "
                 "TEXT, name TEXT, severity TEXT, message TEXT, description TEXT, device_manifest_id INTEGER, "
                 "FOREIGN KEY(device_manifest_id) REFERENCES device_manifest(id) ON DELETE CASCADE);";

    // Actuator manifest
    statement << "CREATE TABLE IF NOT EXISTS actuator_manifest (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                 "reference TEXT, name TEXT, description TEXT, unit TEXT, reading_type TEXT, data_type TEXT, "
                 "precision INTEGER, minimum REAL, maximum REAL, delimiter TEXT, device_manifest_id INTEGER, "
                 "FOREIGN KEY(device_manifest_id) REFERENCES device_manifest(id) ON DELETE CASCADE);";

    statement << "CREATE TABLE IF NOT EXISTS actuator_label (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, label "
                 "TEXT, actuator_manifest_id INTEGER, "
                 "CONSTRAINT k FOREIGN KEY(actuator_manifest_id) REFERENCES actuator_manifest(id) ON DELETE CASCADE);";

    // Sensor manifest
    statement << "CREATE TABLE IF NOT EXISTS sensor_manifest (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, reference "
                 "TEXT, name TEXT, description TEXT, unit TEXT, reading_type TEXT, data_type TEXT, "
                 "precision INTEGER, minimum REAL, maximum REAL, delimiter TEXT, device_manifest_id INTEGER, "
                 "FOREIGN KEY(device_manifest_id) REFERENCES device_manifest(id) ON DELETE CASCADE);";

    statement << "CREATE TABLE IF NOT EXISTS sensor_label (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, label TEXT, "
                 "sensor_manifest_id INTEGER, "
                 "CONSTRAINT k FOREIGN KEY(sensor_manifest_id) REFERENCES sensor_manifest(id) ON DELETE CASCADE);";

    // Configuration manifest
    statement << "CREATE TABLE IF NOT EXISTS configuration_manifest (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, "
                 "reference TEXT, name TEXT, description TEXT, "
                 "unit TEXT, data_type TEXT, minimum REAL, maximum REAL, size INTEGER, delimiter TEXT, collapse_key "
                 "TEXT, default_value TEXT, null_value TEXT, optional TEXT, device_manifest_id INTEGER, "
                 "FOREIGN KEY(device_manifest_id) REFERENCES device_manifest(id) ON DELETE CASCADE);";

    // Device manifest
    statement << "CREATE TABLE IF NOT EXISTS device_manifest (id INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, name "
                 "TEXT, description TEXT, protocol TEXT, firmware_update_protocol TEXT);";

    // Device
    statement
      << "CREATE TABLE IF NOT EXISTS device (key TEXT PRIMARY KEY, name TEXT, device_manifest_id INTEGER NOT NULL, "
         "FOREIGN KEY(device_manifest_id) REFERENCES device_manifest(id));";

    statement << "PRAGMA foreign_keys=on;";

    statement.execute();
}

void SQLiteDeviceRepository::save(std::shared_ptr<Device> device)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    Statement statement(*m_session);

    Poco::UInt64 devicesWithGivenKeyCount;
    statement << "SELECT count(*) FROM device WHERE device.key=?;", useRef(device->getKey()),
      into(devicesWithGivenKeyCount), now;

    if (devicesWithGivenKeyCount != 0)
    {
        return;
    }

    statement.reset(*m_session);
    statement << "BEGIN TRANSACTION;";

    // Device manifest
    statement
      << "INSERT INTO device_manifest(name, description, protocol, firmware_update_protocol) VALUES(?, ?, ?, ?);",
      useRef(device->getManifest().getName()), useRef(device->getManifest().getDescription()),
      useRef(device->getManifest().getProtocol()), useRef(device->getManifest().getFirmwareUpdateProtocol());

    Poco::UInt64 deviceManifestId;
    statement << "SELECT last_insert_rowid();", into(deviceManifestId);

    // Alarm manifests
    for (const wolkabout::AlarmManifest& alarmManifest : device->getManifest().getAlarms())
    {
        const auto severity = [&]() -> std::string {
            if (alarmManifest.getSeverity() == AlarmManifest::AlarmSeverity::ALERT)
            {
                return "ALERT";
            }
            else if (alarmManifest.getSeverity() == AlarmManifest::AlarmSeverity::CRITICAL)
            {
                return "CRITICAL";
            }
            else if (alarmManifest.getSeverity() == AlarmManifest::AlarmSeverity::ERROR)
            {
                return "ERROR";
            }

            return "";
        }();

        statement << "INSERT INTO alarm_manifest(reference, name, severity, message, description, device_manifest_id) "
                     "VALUES(?, ?, ?, ?, ?, ?);",
          useRef(alarmManifest.getReference()), useRef(alarmManifest.getName()), bind(severity),
          useRef(alarmManifest.getMessage()), useRef(alarmManifest.getDescription()), useRef(deviceManifestId);
    }

    // Actuator manifests
    for (const wolkabout::ActuatorManifest& actuatorManifest : device->getManifest().getActuators())
    {
        const auto dataType = [&]() -> std::string {
            if (actuatorManifest.getDataType() == ActuatorManifest::DataType::BOOLEAN)
            {
                return "BOOLEAN";
            }
            else if (actuatorManifest.getDataType() == ActuatorManifest::DataType::NUMERIC)
            {
                return "NUMERIC";
            }
            else if (actuatorManifest.getDataType() == ActuatorManifest::DataType::STRING)
            {
                return "STRING";
            }

            return "";
        }();

        const auto precision = Poco::UInt32(actuatorManifest.getPrecision());
        const auto minimum = actuatorManifest.getMinimum();
        const auto maximum = actuatorManifest.getMaximum();
        statement << "INSERT INTO actuator_manifest(reference, name, description, unit, reading_type, data_type, "
                     "precision, minimum, maximum, delimiter, device_manifest_id) "
                     "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
          useRef(actuatorManifest.getReference()), useRef(actuatorManifest.getName()),
          useRef(actuatorManifest.getDescription()), useRef(actuatorManifest.getUnit()),
          useRef(actuatorManifest.getReadingType()), bind(dataType), bind(precision), bind(minimum), bind(maximum),
          useRef(actuatorManifest.getDelimiter()), useRef(deviceManifestId);

        for (const std::string& label : actuatorManifest.getLabels())
        {
            statement << "INSERT INTO actuator_label SELECT NULL, ?, id FROM actuator_manifest WHERE "
                         "actuator_manifest.reference=? AND actuator_manifest.device_manifest_id=?;",
              useRef(label), useRef(actuatorManifest.getReference()), useRef(deviceManifestId);
        }
    }

    // Sensor manifests
    for (const wolkabout::SensorManifest& sensorManifest : device->getManifest().getSensors())
    {
        const auto dataType = [&]() -> std::string {
            if (sensorManifest.getDataType() == SensorManifest::DataType::BOOLEAN)
            {
                return "BOOLEAN";
            }
            else if (sensorManifest.getDataType() == SensorManifest::DataType::NUMERIC)
            {
                return "NUMERIC";
            }
            else if (sensorManifest.getDataType() == SensorManifest::DataType::STRING)
            {
                return "STRING";
            }

            return "";
        }();

        const auto precision = Poco::UInt32(sensorManifest.getPrecision());
        const auto minimum = sensorManifest.getMinimum();
        const auto maximum = sensorManifest.getMaximum();
        statement << "INSERT INTO sensor_manifest(reference, name, description, unit, reading_type, data_type, "
                     "precision, minimum, maximum, delimiter, device_manifest_id) "
                     "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
          useRef(sensorManifest.getReference()), useRef(sensorManifest.getName()),
          useRef(sensorManifest.getDescription()), useRef(sensorManifest.getUnit()),
          useRef(sensorManifest.getReadingType()), bind(dataType), bind(precision), bind(minimum), bind(maximum),
          useRef(sensorManifest.getDelimiter()), useRef(deviceManifestId);

        for (const std::string& label : sensorManifest.getLabels())
        {
            statement << "INSERT INTO sensor_label SELECT NULL, ?, id FROM sensor_manifest WHERE "
                         "sensor_manifest.reference=? AND sensor_manifest.device_manifest_id=?;",
              useRef(label), useRef(sensorManifest.getReference()), useRef(deviceManifestId);
        }
    }

    // Configuration manifests
    for (const wolkabout::ConfigurationManifest& configurationManifest : device->getManifest().getConfigurations())
    {
        const auto dataType = [&]() -> std::string {
            if (configurationManifest.getDataType() == ConfigurationManifest::DataType::BOOLEAN)
            {
                return "BOOLEAN";
            }
            else if (configurationManifest.getDataType() == ConfigurationManifest::DataType::NUMERIC)
            {
                return "NUMERIC";
            }
            else if (configurationManifest.getDataType() == ConfigurationManifest::DataType::STRING)
            {
                return "STRING";
            }

            return "";
        }();

        const auto minimum = configurationManifest.getMinimum();
        const auto maximum = configurationManifest.getMaximum();
        const auto size = Poco::UInt32(configurationManifest.getSize());
        const auto isOptional = Poco::UInt8(configurationManifest.isOptional() ? 1 : 0);

        statement << "INSERT INTO configuration_manifest(reference, name, description, unit, data_type, minimum, "
                     "maximum, size, delimiter, collapse_key, default_value, null_value, optional, device_manifest_id)"
                     "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
          useRef(configurationManifest.getReference()), useRef(configurationManifest.getName()),
          useRef(configurationManifest.getDescription()), useRef(configurationManifest.getUnit()), bind(dataType),
          bind(minimum), bind(maximum), bind(size), useRef(configurationManifest.getDelimiter()),
          useRef(configurationManifest.getCollapseKey()), useRef(configurationManifest.getDefaultValue()),
          useRef(configurationManifest.getNullValue()), bind(isOptional), useRef(deviceManifestId);
    }

    // Device
    statement << "INSERT INTO device(key, name, device_manifest_id) VALUES(?, ?, ?);", useRef(device->getKey()),
      useRef(device->getName()), useRef(deviceManifestId);
    statement << "COMMIT;", now;
}

void SQLiteDeviceRepository::update(std::shared_ptr<Device> device)
{
    Statement statement(*m_session);

    Poco::UInt64 deviceManifestId;
    statement << "SELECT device_manifest_id FROM device WHERE device.key=?;", useRef(device->getKey()),
      into(deviceManifestId);
    if (statement.execute() == 0)
    {
        return;
    }

    statement.reset(*m_session);
    statement << "BEGIN TRANSACTION;";
    statement << "DELETE FROM device            WHERE device.key=?;", useRef(device->getKey());
    statement << "DELETE FROM device_manifest   WHERE device_manifest.id=?;", useRef(deviceManifestId);

    // Device manifest
    statement
      << "INSERT INTO device_manifest(name, description, protocol, firmware_update_protocol) VALUES(?, ?, ?, ?);",
      useRef(device->getManifest().getName()), useRef(device->getManifest().getDescription()),
      useRef(device->getManifest().getProtocol()), useRef(device->getManifest().getFirmwareUpdateProtocol());

    statement << "SELECT last_insert_rowid();", into(deviceManifestId);

    // Alarm manifests
    for (const wolkabout::AlarmManifest& alarmManifest : device->getManifest().getAlarms())
    {
        const auto severity = [&]() -> std::string {
            if (alarmManifest.getSeverity() == AlarmManifest::AlarmSeverity::ALERT)
            {
                return "ALERT";
            }
            else if (alarmManifest.getSeverity() == AlarmManifest::AlarmSeverity::CRITICAL)
            {
                return "CRITICAL";
            }
            else if (alarmManifest.getSeverity() == AlarmManifest::AlarmSeverity::ERROR)
            {
                return "ERROR";
            }

            return "";
        }();

        statement << "INSERT INTO alarm_manifest(reference, name, severity, message, description, device_manifest_id) "
                     "VALUES(?, ?, ?, ?, ?, ?);",
          useRef(alarmManifest.getReference()), useRef(alarmManifest.getName()), bind(severity),
          useRef(alarmManifest.getMessage()), useRef(alarmManifest.getDescription()), useRef(deviceManifestId);
    }

    // Actuator manifests
    for (const wolkabout::ActuatorManifest& actuatorManifest : device->getManifest().getActuators())
    {
        const auto dataType = [&]() -> std::string {
            if (actuatorManifest.getDataType() == ActuatorManifest::DataType::BOOLEAN)
            {
                return "BOOLEAN";
            }
            else if (actuatorManifest.getDataType() == ActuatorManifest::DataType::NUMERIC)
            {
                return "NUMERIC";
            }
            else if (actuatorManifest.getDataType() == ActuatorManifest::DataType::STRING)
            {
                return "STRING";
            }

            return "";
        }();

        const auto precision = Poco::UInt32(actuatorManifest.getPrecision());
        const auto minimum = actuatorManifest.getMinimum();
        const auto maximum = actuatorManifest.getMaximum();
        statement << "INSERT INTO actuator_manifest(reference, name, description, unit, reading_type, data_type, "
                     "precision, minimum, maximum, delimiter, device_manifest_id) "
                     "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
          useRef(actuatorManifest.getReference()), useRef(actuatorManifest.getName()),
          useRef(actuatorManifest.getDescription()), useRef(actuatorManifest.getUnit()),
          useRef(actuatorManifest.getReadingType()), bind(dataType), bind(precision), bind(minimum), bind(maximum),
          useRef(actuatorManifest.getDelimiter()), useRef(deviceManifestId);

        for (const std::string& label : actuatorManifest.getLabels())
        {
            statement << "INSERT INTO actuator_label SELECT NULL, ?, id FROM actuator_manifest WHERE "
                         "actuator_manifest.reference=? AND actuator_manifest.device_manifest_id=?;",
              useRef(label), useRef(actuatorManifest.getReference()), useRef(deviceManifestId);
        }
    }

    // Sensor manifests
    for (const wolkabout::SensorManifest& sensorManifest : device->getManifest().getSensors())
    {
        const auto dataType = [&]() -> std::string {
            if (sensorManifest.getDataType() == SensorManifest::DataType::BOOLEAN)
            {
                return "BOOLEAN";
            }
            else if (sensorManifest.getDataType() == SensorManifest::DataType::NUMERIC)
            {
                return "NUMERIC";
            }
            else if (sensorManifest.getDataType() == SensorManifest::DataType::STRING)
            {
                return "STRING";
            }

            return "";
        }();

        const auto precision = Poco::UInt32(sensorManifest.getPrecision());
        const auto minimum = sensorManifest.getMinimum();
        const auto maximum = sensorManifest.getMaximum();
        statement << "INSERT INTO sensor_manifest(reference, name, description, unit, reading_type, data_type, "
                     "precision, minimum, maximum, delimiter, device_manifest_id) "
                     "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
          useRef(sensorManifest.getReference()), useRef(sensorManifest.getName()),
          useRef(sensorManifest.getDescription()), useRef(sensorManifest.getUnit()),
          useRef(sensorManifest.getReadingType()), bind(dataType), bind(precision), bind(minimum), bind(maximum),
          useRef(sensorManifest.getDelimiter()), useRef(deviceManifestId);

        for (const std::string& label : sensorManifest.getLabels())
        {
            statement << "INSERT INTO sensor_label SELECT NULL, ?, id FROM sensor_manifest WHERE "
                         "sensor_manifest.reference=? AND sensor_manifest.device_manifest_id=?;",
              useRef(label), useRef(sensorManifest.getReference()), useRef(deviceManifestId);
        }
    }

    // Configuration manifests
    for (const wolkabout::ConfigurationManifest& configurationManifest : device->getManifest().getConfigurations())
    {
        const auto dataType = [&]() -> std::string {
            if (configurationManifest.getDataType() == ConfigurationManifest::DataType::BOOLEAN)
            {
                return "BOOLEAN";
            }
            else if (configurationManifest.getDataType() == ConfigurationManifest::DataType::NUMERIC)
            {
                return "NUMERIC";
            }
            else if (configurationManifest.getDataType() == ConfigurationManifest::DataType::STRING)
            {
                return "STRING";
            }

            return "";
        }();

        const auto minimum = Poco::Int64(configurationManifest.getMinimum());
        const auto maximum = Poco::Int64(configurationManifest.getMaximum());
        const auto size = Poco::UInt64(configurationManifest.getSize());
        const auto isOptional = Poco::UInt8(configurationManifest.isOptional() ? 1 : 0);
        statement << "INSERT INTO configuration_manifest(reference, name, description, unit, data_type, minimum, "
                     "maximum, size, delimiter, collapse_key, default_value, null_value, optional, device_manifest_id)"
                     "VALUES(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);",
          useRef(configurationManifest.getReference()), useRef(configurationManifest.getName()),
          useRef(configurationManifest.getDescription()), useRef(configurationManifest.getUnit()), bind(dataType),
          bind(minimum), bind(maximum), bind(size), useRef(configurationManifest.getDelimiter()),
          useRef(configurationManifest.getCollapseKey()), useRef(configurationManifest.getDefaultValue()),
          useRef(configurationManifest.getNullValue()), bind(isOptional), useRef(deviceManifestId);
    }

    // Device
    statement << "INSERT INTO device(key, name, device_manifest_id) VALUES(?, ?, ?);", useRef(device->getKey()),
      useRef(device->getName()), useRef(deviceManifestId);
    statement << "COMMIT;", now;
}

void SQLiteDeviceRepository::remove(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    Statement statement(*m_session);

    Poco::UInt64 deviceManifestId;
    statement << "SELECT device_manifest_id FROM device WHERE device.key=?;", useRef(deviceKey), into(deviceManifestId);
    if (statement.execute() == 0)
    {
        return;
    }

    statement.reset(*m_session);
    statement << "BEGIN TRANSACTION;";

    statement << "DELETE FROM device                    WHERE device.key=?;", useRef(deviceKey);
    statement << "DELETE FROM device_manifest           WHERE device_manifest.id=?;", useRef(deviceManifestId);

    statement << "COMMIT;", now;
}

std::shared_ptr<Device> SQLiteDeviceRepository::findByDeviceKey(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    Poco::UInt64 deviceManifestId;
    std::string deviceName;
    Statement statement(*m_session);
    statement << "SELECT name, device_manifest_id FROM device WHERE device.key=?;", useRef(deviceKey), into(deviceName),
      into(deviceManifestId);
    if (statement.execute() == 0)
    {
        return nullptr;
    }

    // Device manifest
    std::string manifestName;
    std::string manifestDescription;
    std::string protocol;
    std::string firmwareUpdateProtocol;

    statement.reset(*m_session);
    statement << "SELECT name, description, protocol, firmware_update_protocol FROM device_manifest WHERE id=?;",
      useRef(deviceManifestId), into(manifestName), into(manifestDescription), into(protocol),
      into(firmwareUpdateProtocol), now;

    auto deviceManifest = std::unique_ptr<DeviceManifest>(
      new DeviceManifest(manifestName, manifestDescription, protocol, firmwareUpdateProtocol));

    // Alarm manifests
    std::string alarmReference;
    std::string alarmName;
    std::string alarmSeverityStr;
    std::string alarmMessage;
    std::string alarmDescription;
    statement.reset(*m_session);
    statement
      << "SELECT reference, name, severity, message, description FROM alarm_manifest WHERE device_manifest_id=?;",
      useRef(deviceManifestId), into(alarmReference), into(alarmName), into(alarmSeverityStr), into(alarmMessage),
      into(alarmDescription), range(0, 1);

    while (!statement.done())
    {
        statement.execute();

        const auto alarmSeverity = [&]() -> AlarmManifest::AlarmSeverity {
            if (alarmSeverityStr == "ALERT")
            {
                return AlarmManifest::AlarmSeverity::ALERT;
            }
            else if (alarmSeverityStr == "CRITICAL")
            {
                return AlarmManifest::AlarmSeverity::CRITICAL;
            }
            else if (alarmSeverityStr == "ERROR")
            {
                return AlarmManifest::AlarmSeverity::ERROR;
            }
            return AlarmManifest::AlarmSeverity::ALERT;
        }();

        deviceManifest->addAlarm(
          AlarmManifest(alarmName, alarmSeverity, alarmReference, alarmMessage, alarmDescription));
    }

    // Actuator manifests
    Poco::UInt64 actuatorManifestId;
    std::string actuatorReference;
    std::string actuatorName;
    std::string actuatorDescription;
    std::string actuatorUnit;
    std::string actuatorReadingType;
    std::string actuatorDataTypeStr;
    Poco::UInt32 actuatorPrecision;
    double actuatorMinimum;
    double actuatorMaximum;
    std::string actuatorDelimiter;
    statement.reset(*m_session);
    statement << "SELECT id, reference, name, description, unit, reading_type, data_type, precision, minimum, maximum, "
                 "delimiter "
                 "FROM actuator_manifest WHERE device_manifest_id=?;",
      useRef(deviceManifestId), into(actuatorManifestId), into(actuatorReference), into(actuatorName),
      into(actuatorDescription), into(actuatorUnit), into(actuatorReadingType), into(actuatorDataTypeStr),
      into(actuatorPrecision), into(actuatorMinimum), into(actuatorMaximum), into(actuatorDelimiter), range(0, 1);

    while (!statement.done())
    {
        statement.execute();

        const auto actuatorDataType = [&]() -> ActuatorManifest::DataType {
            if (actuatorDataTypeStr == "BOOLEAN")
            {
                return ActuatorManifest::DataType::BOOLEAN;
            }
            else if (actuatorDataTypeStr == "NUMERIC")
            {
                return ActuatorManifest::DataType::NUMERIC;
            }
            else if (actuatorDataTypeStr == "STRING")
            {
                return ActuatorManifest::DataType::STRING;
            }

            return ActuatorManifest::DataType::STRING;
        }();

        std::vector<std::string> labels;
        Statement selectLabelsStatement(*m_session);
        selectLabelsStatement << "SELECT label FROM actuator_label WHERE actuator_manifest_id=?;",
          bind(actuatorManifestId), into(labels), now;

        deviceManifest->addActuator(ActuatorManifest(actuatorName, actuatorReference, actuatorDescription, actuatorUnit,
                                                     actuatorReadingType, actuatorDataType, actuatorPrecision,
                                                     actuatorMinimum, actuatorMaximum, actuatorDelimiter, labels));
    }

    // Sensor manifests
    Poco::UInt64 sensorManifestId;
    std::string sensorReference;
    std::string sensorName;
    std::string sensorDescription;
    std::string sensorUnit;
    std::string sensorReadingType;
    std::string sensorDataTypeStr;
    Poco::UInt32 sensorPrecision;
    double sensorMinimum;
    double sensorMaximum;
    std::string sensorDelimiter;
    statement.reset(*m_session);
    statement << "SELECT id, reference, name, description, unit, reading_type, data_type, precision, minimum, maximum, "
                 "delimiter "
                 "FROM sensor_manifest WHERE device_manifest_id=?;",
      useRef(deviceManifestId), into(sensorManifestId), into(sensorReference), into(sensorName),
      into(sensorDescription), into(sensorUnit), into(sensorReadingType), into(sensorDataTypeStr),
      into(sensorPrecision), into(sensorMinimum), into(sensorMaximum), into(sensorDelimiter), range(0, 1);

    while (!statement.done())
    {
        statement.execute();

        const auto sensorDataType = [&]() -> SensorManifest::DataType {
            if (sensorDataTypeStr == "BOOLEAN")
            {
                return SensorManifest::DataType::BOOLEAN;
            }
            else if (sensorDataTypeStr == "NUMERIC")
            {
                return SensorManifest::DataType::NUMERIC;
            }
            else if (sensorDataTypeStr == "STRING")
            {
                return SensorManifest::DataType::STRING;
            }

            return SensorManifest::DataType::STRING;
        }();

        std::vector<std::string> labels;
        Statement selectLabelsStatement(*m_session);
        selectLabelsStatement << "SELECT label FROM sensor_label WHERE sensor_manifest_id=?;", bind(sensorManifestId),
          into(labels), now;

        deviceManifest->addSensor(SensorManifest(sensorName, sensorReference, sensorDescription, sensorUnit,
                                                 sensorReadingType, sensorDataType, sensorPrecision, sensorMinimum,
                                                 sensorMaximum, sensorDelimiter, labels));
    }

    // Configuration manifests
    std::string configurationReference;
    std::string configurationName;
    std::string configurationDescription;
    std::string configurationUnit;
    std::string configurationDataTypeStr;
    double configurationMinimum;
    double configurationMaximum;
    Poco::UInt32 configurationSize;
    std::string configurationDelimiter;
    std::string configurationCollapseKey;
    std::string configurationDefaultValue;
    std::string configurationNullValue;
    Poco::UInt8 configurationIsOptional;
    statement.reset(*m_session);
    statement << "SELECT reference, name, description, unit, data_type, minimum, maximum, size, delimiter, "
                 "collapse_key, default_value, null_value,"
                 "optional FROM configuration_manifest WHERE device_manifest_id=?;",
      useRef(deviceManifestId), into(configurationReference), into(configurationName), into(configurationDescription),
      into(configurationUnit), into(configurationDataTypeStr), into(configurationMinimum), into(configurationMaximum),
      into(configurationSize), into(configurationDelimiter), into(configurationCollapseKey),
      into(configurationDefaultValue), into(configurationNullValue), into(configurationIsOptional), range(0, 1);

    while (!statement.done())
    {
        statement.execute();

        const auto configurationDataType = [&]() -> ConfigurationManifest::DataType {
            if (configurationDataTypeStr == "STRING")
            {
                return ConfigurationManifest::DataType::STRING;
            }
            else if (configurationDataTypeStr == "BOOLEAN")
            {
                return ConfigurationManifest::DataType::BOOLEAN;
            }
            else if (configurationDataTypeStr == "NUMERIC")
            {
                return ConfigurationManifest::DataType::NUMERIC;
            }

            return ConfigurationManifest::DataType::STRING;
        }();

        deviceManifest->addConfiguration(ConfigurationManifest(
          configurationName, configurationReference, configurationDescription, configurationUnit, configurationDataType,
          configurationMinimum, configurationMaximum, configurationCollapseKey, configurationDefaultValue,
          configurationNullValue, configurationIsOptional != 0 ? true : false, configurationSize,
          configurationDelimiter));
    }

    return std::make_shared<Device>(deviceName, deviceKey, *deviceManifest);
}

std::shared_ptr<std::vector<std::string>> SQLiteDeviceRepository::findAllDeviceKeys()
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    auto deviceKeys = std::make_shared<std::vector<std::string>>();

    Statement statement(*m_session);
    statement << "SELECT key FROM device;", into(*deviceKeys), now;

    return deviceKeys;
}
}    // namespace wolkabout
