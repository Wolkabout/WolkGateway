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

    bool operator==(DeviceManifest& rhs) const;
    bool operator!=(DeviceManifest& rhs) const;

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
