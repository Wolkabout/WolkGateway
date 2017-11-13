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

#ifndef JSONPERSISTSERVICE_H
#define JSONPERSISTSERVICE_H

#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/Reading.h"
#include "model/SensorReading.h"
#include "service/persist/PersistService.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
class Reading;
class JsonPersistService : public PersistService
{
public:
    JsonPersistService(std::string persistPath = "persistence",
                       unsigned long long int maximumNumberOfPersistedItems = 0, bool isCircular = false);
    virtual ~JsonPersistService() = default;

    bool hasPersistedReadings() override;

    void persist(std::shared_ptr<Reading> reading) override;

    std::shared_ptr<Reading> unpersistFirst() override;
    void dropFirst() override;

private:
    void persist(const ActuatorStatus& actuatorStatus);
    void persist(const SensorReading& sensorReading);
    void persist(const Alarm& alarm);

    std::shared_ptr<Reading> unpersistActuatorStatus(const std::string& readingFile);
    std::shared_ptr<Reading> unpersistSensorReading(const std::string& readingFile);
    std::shared_ptr<Reading> unpersistAlarm(const std::string& readingFile);

    std::string generateFileName(const ActuatorStatus& actuatorStatus);
    std::string generateFileName(const SensorReading& sensorReading);
    std::string generateFileName(const Alarm& alarm);

    const std::vector<std::string>& getPersistedActuatorStatusFilenames(bool forceCacheReload = false);
    const std::vector<std::string>& getPersistedSensorReadingFilenames(bool forceCacheReload = false);
    const std::vector<std::string>& getPersistedAlarmFilenames(bool forceCacheReload = false);

    std::vector<std::string> getPersistedItemsFilenames(bool forceCacheReload = false);

    void invalidatePersistedItemsCache();

    // std::pair<Is list dirty, Readings list>
    std::pair<bool, std::vector<std::string>> m_cachedActuatorStatusFilenames;
    std::pair<bool, std::vector<std::string>> m_cachedSensorReadingFilenames;
    std::pair<bool, std::vector<std::string>> m_cachedAlarmFilenames;

    static const constexpr int PersistedItemsListIsDirty = 0;
    static const constexpr int PersistedItemsList = 1;

    static const constexpr char* ACTUATOR_STATUS_DIRECTORY = "actuator_status_dir/";
    static const constexpr char* SENSOR_READING_DIRECTORY = "sensor_reading_dir/";
    static const constexpr char* ALARM_DIRECTORY = "alarm_dir/";
};
}

#endif
