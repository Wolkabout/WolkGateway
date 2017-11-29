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

#ifndef INMEMORYPERSISTENCE_H
#define INMEMORYPERSISTENCE_H

#include "persistence/Persistence.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace wolkabout
{
class Reading;
class Alarm;
class ActuatorStatus;
class InMemoryPersistence : public Persistence
{
public:
    InMemoryPersistence() = default;
    virtual ~InMemoryPersistence() = default;

    bool putSensorReading(const std::string& key, std::shared_ptr<SensorReading> sensorReading) override;
    std::vector<std::shared_ptr<SensorReading>> getSensorReadings(const std::string& key, uint_fast64_t count) override;
    void removeSensorReadings(const std::string& key, uint_fast64_t count) override;
    std::vector<std::string> getSensorReadingsKeys() override;

    bool putAlarm(const std::string& key, std::shared_ptr<Alarm> alarm) override;
    std::vector<std::shared_ptr<Alarm>> getAlarms(const std::string& key, uint_fast64_t count) override;
    void removeAlarms(const std::string& key, uint_fast64_t count) override;
    std::vector<std::string> getAlarmsKeys() override;

    bool putActuatorStatus(const std::string& key, std::shared_ptr<ActuatorStatus> actuatorStatus) override;
    std::shared_ptr<ActuatorStatus> getActuatorStatus(const std::string& key) override;
    void removeActuatorStatus(const std::string& key) override;
    std::vector<std::string> getGetActuatorStatusesKeys() override;

    bool isEmpty() override;

private:
    std::vector<std::shared_ptr<SensorReading>>& getOrCreateSensorReadingsByKey(const std::string& key);
    std::vector<std::shared_ptr<Alarm>>& getOrCreateAlarmsByKey(const std::string& key);

    std::map<std::string, std::vector<std::shared_ptr<SensorReading>>> m_readings;
    std::map<std::string, std::vector<std::shared_ptr<Alarm>>> m_alarms;
    std::map<std::string, std::shared_ptr<ActuatorStatus>> m_actuatorStatuses;
};
}

#endif
