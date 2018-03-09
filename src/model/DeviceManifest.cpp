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

#include "model/DeviceManifest.h"
#include "model/ActuatorManifest.h"
#include "model/AlarmManifest.h"
#include "model/ConfigurationManifest.h"
#include "model/SensorManifest.h"

#include <functional>
#include <string>
#include <vector>

namespace wolkabout
{
DeviceManifest::DeviceManifest(std::string name, std::string description, std::string protocol,
                               std::string firmwareUpdateProtocol, std::vector<ConfigurationManifest> configurations,
                               std::vector<SensorManifest> sensors, std::vector<AlarmManifest> alarms,
                               std::vector<ActuatorManifest> actuators)
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

DeviceManifest& wolkabout::DeviceManifest::addConfiguration(
  const wolkabout::ConfigurationManifest& configurationManifest)
{
    m_configurations.push_back(configurationManifest);
    return *this;
}

DeviceManifest& DeviceManifest::addSensor(const SensorManifest& sensorManifest)
{
    m_sensors.push_back(sensorManifest);
    return *this;
}

DeviceManifest& DeviceManifest::addAlarm(const AlarmManifest& alarmManifest)
{
    m_alarms.push_back(alarmManifest);
    return *this;
}

DeviceManifest& DeviceManifest::addActuator(const ActuatorManifest& actuatorManifest)
{
    m_actuators.push_back(actuatorManifest);
    return *this;
}

const std::vector<ConfigurationManifest>& DeviceManifest::getConfigurations() const
{
    return m_configurations;
}

const std::vector<SensorManifest>& DeviceManifest::getSensors() const
{
    return m_sensors;
}

const std::vector<AlarmManifest>& DeviceManifest::getAlarms() const
{
    return m_alarms;
}

const std::vector<ActuatorManifest>& DeviceManifest::getActuators() const
{
    return m_actuators;
}

std::unique_ptr<ConfigurationManifest> DeviceManifest::getConfigurationManifest(
  std::function<bool(const ConfigurationManifest&)> filter) const
{
    for (const ConfigurationManifest& configurationManifest : m_configurations)
    {
        if (filter(configurationManifest))
        {
            return std::unique_ptr<ConfigurationManifest>(new ConfigurationManifest(configurationManifest));
        }
    }

    return nullptr;
}

std::unique_ptr<SensorManifest> DeviceManifest::getSensorManifest(
  std::function<bool(const SensorManifest&)> filter) const
{
    for (const SensorManifest& sensorManifest : m_sensors)
    {
        if (filter(sensorManifest))
        {
            return std::unique_ptr<SensorManifest>(new SensorManifest(sensorManifest));
        }
    }

    return nullptr;
}

std::unique_ptr<AlarmManifest> DeviceManifest::getAlarmManifest(std::function<bool(const AlarmManifest&)> filter) const
{
    for (const AlarmManifest& alarmManifest : m_alarms)
    {
        if (filter(alarmManifest))
        {
            return std::unique_ptr<AlarmManifest>(new AlarmManifest(alarmManifest));
        }
    }

    return nullptr;
}

std::unique_ptr<ActuatorManifest> DeviceManifest::getActuatorManifest(
  std::function<bool(const ActuatorManifest&)> filter) const
{
    for (const ActuatorManifest& actuatorManifest : m_actuators)
    {
        if (filter(actuatorManifest))
        {
            return std::unique_ptr<ActuatorManifest>(new ActuatorManifest(actuatorManifest));
        }
    }

    return nullptr;
}

bool DeviceManifest::hasConfigurationManifestWithReference(const std::string& reference) const
{
    for (const ConfigurationManifest& configurationManifest : m_configurations)
    {
        if (configurationManifest.getReference() == reference)
        {
            return true;
        }
    }

    return false;
}

bool DeviceManifest::hasSensorManifestWithReference(const std::string& reference) const
{
    for (const SensorManifest& sensorManifest : m_sensors)
    {
        if (sensorManifest.getReference() == reference)
        {
            return true;
        }
    }

    return false;
}

bool DeviceManifest::hasAlarmManifestWithReference(const std::string& reference) const
{
    for (const AlarmManifest& alarmManifest : m_alarms)
    {
        if (alarmManifest.getReference() == reference)
        {
            return true;
        }
    }

    return false;
}

bool DeviceManifest::hasActuatorManifestWithReference(const std::string& reference) const
{
    for (const ActuatorManifest& actuatorManifest : m_actuators)
    {
        if (actuatorManifest.getReference() == reference)
        {
            return true;
        }
    }

    return false;
}

const std::string& DeviceManifest::getName() const
{
    return m_name;
}

const std::string& DeviceManifest::getDescription() const
{
    return m_description;
}

const std::string& DeviceManifest::getFirmwareUpdateProtocol() const
{
    return m_firmwareUpdateProtocol;
}

bool DeviceManifest::operator==(DeviceManifest& rhs) const
{
    if (m_name != rhs.m_name || m_description != rhs.m_description || m_protocol != rhs.m_protocol ||
        m_firmwareUpdateProtocol != rhs.m_firmwareUpdateProtocol)
    {
        return false;
    }

    if (m_configurations.size() != rhs.m_configurations.size())
    {
        return false;
    }

    if (m_sensors.size() != rhs.m_sensors.size())
    {
        return false;
    }

    if (m_actuators.size() != rhs.m_actuators.size())
    {
        return false;
    }

    if (m_alarms.size() != rhs.m_alarms.size())
    {
        return false;
    }

    for (unsigned long long int i = 0; i < m_configurations.size(); ++i)
    {
        if (m_configurations.at(i) != rhs.m_configurations.at(i))
        {
            return false;
        }
    }

    for (unsigned long long int i = 0; i < m_sensors.size(); ++i)
    {
        if (m_sensors.at(i) != rhs.m_sensors.at(i))
        {
            return false;
        }
    }

    for (unsigned long long int i = 0; i < m_actuators.size(); ++i)
    {
        if (m_actuators.at(i) != rhs.m_actuators.at(i))
        {
            return false;
        }
    }

    for (unsigned long long int i = 0; i < m_alarms.size(); ++i)
    {
        if (m_alarms.at(i) != rhs.m_alarms.at(i))
        {
            return false;
        }
    }

    return true;
}

bool DeviceManifest::operator!=(DeviceManifest& rhs) const
{
    return !(*this == rhs);
}

const std::string& DeviceManifest::getProtocol() const
{
    return m_protocol;
}
}    // namespace wolkabout
