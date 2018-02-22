#include "model/DeviceManifest.h"
#include "model/ActuatorManifest.h"
#include "model/AlarmManifest.h"
#include "model/ConfigurationManifest.h"
#include "model/SensorManifest.h"

#include <string>
#include <vector>

wolkabout::DeviceManifest::DeviceManifest(std::string name, std::string description, std::string protocol,
                                          std::string firmwareUpdateProtocol,
                                          std::vector<wolkabout::ConfigurationManifest> configurations,
                                          std::vector<wolkabout::SensorManifest> sensors,
                                          std::vector<wolkabout::AlarmManifest> alarms,
                                          std::vector<wolkabout::ActuatorManifest> actuators)
: m_name(std::move(name))
, m_description(std::move(description))
, m_protocol(std::move(protocol))
, m_firmwareUpdateProtocol(std::move(firmwareUpdateProtocol))
, m_configurations(std::move(configurations))
, m_sensors(std::move(sensors))
, m_alarms(std::move(alarms))
, m_actuators(std::move(actuators))
{
}

wolkabout::DeviceManifest& wolkabout::DeviceManifest::addConfiguration(
  const wolkabout::ConfigurationManifest& configurationManifest)
{
    m_configurations.push_back(configurationManifest);
    return *this;
}

wolkabout::DeviceManifest& wolkabout::DeviceManifest::addSensor(const wolkabout::SensorManifest& sensorManifest)
{
    m_sensors.push_back(sensorManifest);
    return *this;
}

wolkabout::DeviceManifest& wolkabout::DeviceManifest::addAlarm(const wolkabout::AlarmManifest& alarmManifest)
{
    m_alarms.push_back(alarmManifest);
    return *this;
}

wolkabout::DeviceManifest& wolkabout::DeviceManifest::addActuator(const wolkabout::ActuatorManifest& actuatorManifest)
{
    m_actuators.push_back(actuatorManifest);
    return *this;
}

const std::vector<wolkabout::ConfigurationManifest>& wolkabout::DeviceManifest::getConfigurations() const
{
    return m_configurations;
}

const std::vector<wolkabout::SensorManifest>& wolkabout::DeviceManifest::getSensors() const
{
    return m_sensors;
}

const std::vector<wolkabout::AlarmManifest>& wolkabout::DeviceManifest::getAlarms() const
{
    return m_alarms;
}

const std::vector<wolkabout::ActuatorManifest>& wolkabout::DeviceManifest::getActuators() const
{
    return m_actuators;
}

const std::string& wolkabout::DeviceManifest::getName() const
{
    return m_name;
}

const std::string& wolkabout::DeviceManifest::getDescription() const
{
    return m_description;
}

const std::string& wolkabout::DeviceManifest::getFirmwareUpdateProtocol() const
{
    return m_firmwareUpdateProtocol;
}

const std::string& wolkabout::DeviceManifest::getProtocol() const
{
    return m_protocol;
}
