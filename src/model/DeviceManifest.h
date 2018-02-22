#ifndef MANIFEST_H
#define MANIFEST_H

#include "model/ActuatorManifest.h"
#include "model/AlarmManifest.h"
#include "model/ConfigurationManifest.h"
#include "model/SensorManifest.h"

#include <string>
#include <vector>

namespace wolkabout
{
class DeviceManifest
{
public:
    DeviceManifest() = default;
    DeviceManifest(std::string name, std::string description, std::string protocol, std::string firmwareUpdateProtocol,
                   std::vector<ConfigurationManifest> configurations = {}, std::vector<SensorManifest> sensors = {},
                   std::vector<AlarmManifest> alarms = {}, std::vector<ActuatorManifest> actuators = {});

    virtual ~DeviceManifest() = default;

    DeviceManifest& addConfiguration(const ConfigurationManifest& configurationManifest);
    DeviceManifest& addSensor(const SensorManifest& sensorManifest);
    DeviceManifest& addAlarm(const AlarmManifest& alarmManifest);
    DeviceManifest& addActuator(const ActuatorManifest& actuatorManifest);

    const std::vector<ConfigurationManifest>& getConfigurations() const;
    const std::vector<SensorManifest>& getSensors() const;
    const std::vector<AlarmManifest>& getAlarms() const;
    const std::vector<ActuatorManifest>& getActuators() const;

    const std::string& getName() const;
    const std::string& getDescription() const;
    const std::string& getProtocol() const;
    const std::string& getFirmwareUpdateProtocol() const;

private:
    std::string m_name;
    std::string m_description;
    std::string m_protocol;
    std::string m_firmwareUpdateProtocol;

    std::vector<ConfigurationManifest> m_configurations;
    std::vector<SensorManifest> m_sensors;
    std::vector<AlarmManifest> m_alarms;
    std::vector<ActuatorManifest> m_actuators;
};
}    // namespace wolkabout

#endif    // MANIFEST_H
