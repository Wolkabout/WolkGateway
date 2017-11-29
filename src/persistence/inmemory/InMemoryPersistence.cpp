/*
 * Copyright 2017 WolkAbout Technology s.r.o.
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

#include "persistence/inmemory/InMemoryPersistence.h"
#include "persistence/Persistence.h"

#include <cstddef>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
bool InMemoryPersistence::putSensorReading(const std::string& key, std::shared_ptr<SensorReading> sensorReading)
{
    getOrCreateSensorReadingsByKey(key).push_back(sensorReading);
    return true;
}

std::vector<std::shared_ptr<SensorReading>> InMemoryPersistence::getSensorReadings(const std::string& key,
                                                                                   uint_fast64_t count)
{
    if (m_readings.find(key) == m_readings.end())
    {
        return {};
    }

    std::vector<std::shared_ptr<SensorReading>>& sensorReadings = m_readings.at(key);
    auto size = static_cast<uint_fast64_t>(sensorReadings.size());
    return std::vector<std::shared_ptr<SensorReading>>(sensorReadings.begin(),
                                                       sensorReadings.begin() + (count < size ? count : size));
}

void InMemoryPersistence::removeSensorReadings(const std::string& key, uint_fast64_t count)
{
    if (m_readings.find(key) == m_readings.end())
    {
        return;
    }

    std::vector<std::shared_ptr<SensorReading>>& readings = getOrCreateSensorReadingsByKey(key);
    auto size = static_cast<uint_fast64_t>(readings.size());
    readings.erase(readings.begin(), readings.begin() + (count < size ? count : size));
}

std::vector<std::string> InMemoryPersistence::getSensorReadingsKeys()
{
    std::vector<std::string> keys;
    for (const std::pair<std::string, std::vector<std::shared_ptr<SensorReading>>>& pair : m_readings)
    {
        if (!pair.second.empty())
        {
            keys.push_back(pair.first);
        }
    }

    return keys;
}

bool InMemoryPersistence::putAlarm(const std::string& key, std::shared_ptr<Alarm> alarm)
{
    getOrCreateAlarmsByKey(key).push_back(alarm);
    return true;
}

std::vector<std::shared_ptr<Alarm>> InMemoryPersistence::getAlarms(const std::string& key, uint_fast64_t count)
{
    if (m_alarms.find(key) == m_alarms.end())
    {
        return {};
    }

    std::vector<std::shared_ptr<Alarm>>& alarms = m_alarms.at(key);
    auto size = static_cast<uint_fast64_t>(alarms.size());
    return std::vector<std::shared_ptr<Alarm>>(alarms.begin(), alarms.begin() + (count < size ? count : size));
}

void InMemoryPersistence::removeAlarms(const std::string& key, uint_fast64_t count)
{
    if (m_alarms.find(key) == m_alarms.end())
    {
        return;
    }

    std::vector<std::shared_ptr<Alarm>>& alarms = getOrCreateAlarmsByKey(key);
    auto size = static_cast<uint_fast64_t>(alarms.size());
    alarms.erase(alarms.begin(), alarms.begin() + (count < size ? count : size));
}

std::vector<std::string> InMemoryPersistence::getAlarmsKeys()
{
    std::vector<std::string> keys;
    for (const std::pair<std::string, std::vector<std::shared_ptr<Alarm>>>& pair : m_alarms)
    {
        if (!pair.second.empty())
        {
            keys.push_back(pair.first);
        }
    }

    return keys;
}

bool InMemoryPersistence::putActuatorStatus(const std::string& key, std::shared_ptr<ActuatorStatus> actuatorStatus)
{
    m_actuatorStatuses[key] = actuatorStatus;
    return true;
}

std::shared_ptr<ActuatorStatus> InMemoryPersistence::getActuatorStatus(const std::string& key)
{
    return m_actuatorStatuses.at(key);
}

void InMemoryPersistence::removeActuatorStatus(const std::string& key)
{
    m_actuatorStatuses.erase(key);
}

std::vector<std::string> InMemoryPersistence::getGetActuatorStatusesKeys()
{
    std::vector<std::string> keys;
    for (const std::pair<std::string, std::shared_ptr<ActuatorStatus>>& pair : m_actuatorStatuses)
    {
        keys.push_back(pair.first);
    }

    return keys;
}

bool InMemoryPersistence::isEmpty()
{
    return getSensorReadingsKeys().empty() && m_actuatorStatuses.empty();
}

std::vector<std::shared_ptr<SensorReading>>& InMemoryPersistence::getOrCreateSensorReadingsByKey(const std::string& key)
{
    if (m_readings.find(key) == m_readings.end())
    {
        m_readings.emplace(std::pair<std::string, std::vector<std::shared_ptr<SensorReading>>>(key, {}));
    }

    return m_readings.at(key);
}

std::vector<std::shared_ptr<Alarm>>& InMemoryPersistence::getOrCreateAlarmsByKey(const std::string& key)
{
    if (m_alarms.find(key) == m_alarms.end())
    {
        m_alarms.emplace(std::pair<std::string, std::vector<std::shared_ptr<Alarm>>>(key, {}));
    }

    return m_alarms.at(key);
}
}
