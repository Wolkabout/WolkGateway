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

#include "service/persist/json/JsonPersistService.h"

#include "model/Reading.h"
#include "service/persist/PersistService.h"
#include "service/persist/json/JsonPersistServiceParser.h"
#include "utilities/FileSystemUtils.h"
#include "utilities/StringUtils.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
JsonPersistService::JsonPersistService(std::string persistPath, unsigned long long maximumNumberOfPersistedItems,
                                       bool isCircular)
: PersistService(std::move(persistPath), maximumNumberOfPersistedItems, isCircular)
{
    FileSystemUtils::createDirectory(PersistService::getPersistPath());
    FileSystemUtils::createDirectory(PersistService::getPersistPath() + ACTUATOR_STATUS_DIRECTORY);
    FileSystemUtils::createDirectory(PersistService::getPersistPath() + SENSOR_READING_DIRECTORY);
    FileSystemUtils::createDirectory(PersistService::getPersistPath() + ALARM_DIRECTORY);

    getPersistedItemsFilenames(true);
}

bool JsonPersistService::hasPersistedReadings()
{
    return !getPersistedActuatorStatusFilenames().empty() || !getPersistedSensorReadingFilenames().empty() ||
           !getPersistedAlarmFilenames().empty();
}

void JsonPersistService::persist(std::shared_ptr<Reading> reading)
{
    class ReadingPersisterVisitor final : public ReadingVisitor
    {
    public:
        ReadingPersisterVisitor(JsonPersistService& persistService) : m_persistService(persistService) {}
        ~ReadingPersisterVisitor() = default;

        void visit(ActuatorStatus& actuatorStatus) override { m_persistService.persist(actuatorStatus); }

        void visit(Alarm& alarm) override { m_persistService.persist(alarm); }

        void visit(SensorReading& sensorReading) override { m_persistService.persist(sensorReading); }

    private:
        JsonPersistService& m_persistService;
    } readingSerializerVisitor(*this);
    reading->acceptVisit(readingSerializerVisitor);
}

std::shared_ptr<Reading> JsonPersistService::unpersistFirst()
{
    const std::vector<std::string>& readings = getPersistedItemsFilenames();
    const std::string& readingFile = readings.front();

    if (StringUtils::contains(readingFile, ACTUATOR_STATUS_DIRECTORY))
    {
        return unpersistActuatorStatus(readingFile);
    }
    else if (StringUtils::contains(readingFile, SENSOR_READING_DIRECTORY))
    {
        return unpersistSensorReading(readingFile);
    }
    else if (StringUtils::contains(readingFile, ALARM_DIRECTORY))
    {
        return unpersistAlarm(readingFile);
    }

    return nullptr;
}

void JsonPersistService::dropFirst()
{
    const std::vector<std::string>& readings = getPersistedItemsFilenames();
    const std::string& firstReading = readings.front();

    FileSystemUtils::deleteFile(firstReading);

    invalidatePersistedItemsCache();
}

void JsonPersistService::persist(const ActuatorStatus& actuatorStatus)
{
    std::string name = generateFileName(actuatorStatus);
    std::string content = JsonPersistServiceParser::toJson(actuatorStatus);

    FileSystemUtils::createFileWithContent(name, content);
    invalidatePersistedItemsCache();
}

void JsonPersistService::persist(const SensorReading& sensorReading)
{
    if (PersistService::getMaximumNumberOfPersistedReadings() != 0 &&
        getPersistedSensorReadingFilenames().size() >= PersistService::getMaximumNumberOfPersistedReadings())
    {
        if (PersistService::isCircular())
        {
            FileSystemUtils::deleteFile(getPersistedSensorReadingFilenames().front());
        }
        else
        {
            return;
        }
    }

    std::string name = generateFileName(sensorReading);
    std::string content = JsonPersistServiceParser::toJson(sensorReading);

    FileSystemUtils::createFileWithContent(name, content);
    invalidatePersistedItemsCache();
}

void JsonPersistService::persist(const Alarm& alarm)
{
    if (PersistService::getMaximumNumberOfPersistedReadings() != 0 &&
        getPersistedAlarmFilenames().size() >= PersistService::getMaximumNumberOfPersistedReadings())
    {
        if (PersistService::isCircular())
        {
            FileSystemUtils::deleteFile(getPersistedAlarmFilenames().front());
        }
        else
        {
            return;
        }
    }

    std::string name = generateFileName(alarm);
    std::string content = JsonPersistServiceParser::toJson(alarm);

    FileSystemUtils::createFileWithContent(name, content);
    invalidatePersistedItemsCache();
}

std::shared_ptr<Reading> JsonPersistService::unpersistActuatorStatus(const std::string& readingFile)
{
    std::string readingJson;
    if (!FileSystemUtils::readFileContent(readingFile, readingJson))
    {
        return nullptr;
    }

    ActuatorStatus actuatorStatus;
    if (!JsonPersistServiceParser::fromJson(readingJson, actuatorStatus))
    {
        return nullptr;
    }

    return std::make_shared<ActuatorStatus>(actuatorStatus);
}

std::shared_ptr<Reading> JsonPersistService::unpersistSensorReading(const std::string& readingFile)
{
    std::string readingJson;
    if (!FileSystemUtils::readFileContent(readingFile, readingJson))
    {
        return nullptr;
    }

    SensorReading sensorReading;
    if (!JsonPersistServiceParser::fromJson(readingJson, sensorReading))
    {
        return nullptr;
    }

    return std::make_shared<SensorReading>(sensorReading);
}

std::shared_ptr<Reading> JsonPersistService::unpersistAlarm(const std::string& readingFile)
{
    std::string readingJson;
    if (!FileSystemUtils::readFileContent(readingFile, readingJson))
    {
        return nullptr;
    }

    Alarm alarm;
    if (!JsonPersistServiceParser::fromJson(readingJson, alarm))
    {
        return nullptr;
    }

    return std::make_shared<Alarm>(alarm);
}

std::string JsonPersistService::generateFileName(const ActuatorStatus& actuatorStatus)
{
    return PersistService::getPersistPath() + ACTUATOR_STATUS_DIRECTORY + actuatorStatus.getReference();
}

std::string JsonPersistService::generateFileName(const SensorReading& sensorReading)
{
    unsigned long long readingNumber = 0;
    const std::vector<std::string>& sensorReadings = getPersistedSensorReadingFilenames();
    if (sensorReadings.size() != 0)
    {
        const std::string& lastReading = sensorReadings.back();

        try
        {
            readingNumber = std::stoull(StringUtils::tokenize(lastReading, "/").back());
        }
        catch (std::invalid_argument&)
        {
            FileSystemUtils::deleteFile(lastReading);
            invalidatePersistedItemsCache();
            return generateFileName(sensorReading);
        }
    }

    return PersistService::getPersistPath() + SENSOR_READING_DIRECTORY + std::to_string(readingNumber + 1) + "-" +
           sensorReading.getReference();
}

std::string JsonPersistService::generateFileName(const Alarm& alarm)
{
    unsigned long long alarmNumber = 0;
    const std::vector<std::string>& alarms = getPersistedAlarmFilenames();
    if (alarms.size() != 0)
    {
        const std::string& lastAlarm = alarms.back();
        try
        {
            alarmNumber = std::stoull(StringUtils::tokenize(lastAlarm, "/").back());
        }
        catch (std::invalid_argument&)
        {
            FileSystemUtils::deleteFile(lastAlarm);
            invalidatePersistedItemsCache();
            return generateFileName(alarm);
        }
    }

    return PersistService::getPersistPath() + ALARM_DIRECTORY + std::to_string(alarmNumber + 1) + "-" +
           alarm.getReference();
}

const std::vector<std::string>& JsonPersistService::getPersistedActuatorStatusFilenames(bool forceCacheReload)
{
    if (forceCacheReload || std::get<PersistedItemsListIsDirty>(m_cachedActuatorStatusFilenames))
    {
        std::vector<std::string> actuatorStatuses;
        for (const std::string& actuatorStatus :
             FileSystemUtils::listFiles(PersistService::getPersistPath() + ACTUATOR_STATUS_DIRECTORY))
        {
            actuatorStatuses.emplace_back(PersistService::getPersistPath() + ACTUATOR_STATUS_DIRECTORY +
                                          actuatorStatus);
        }

        std::sort(actuatorStatuses.begin(), actuatorStatuses.end(),
                  [](const std::string& firstFilename, const std::string& secondFilename) -> bool {
                      try
                      {
                          auto a = std::stoull(StringUtils::tokenize(firstFilename, "/").back());
                          auto b = std::stoull(StringUtils::tokenize(secondFilename, "/").back());

                          return a < b;
                      }
                      catch (std::invalid_argument&)
                      {
                          return false;
                      }
                  });

        std::get<PersistedItemsList>(m_cachedActuatorStatusFilenames) = std::move(actuatorStatuses);
        std::get<PersistedItemsListIsDirty>(m_cachedActuatorStatusFilenames) = false;
    }

    return std::get<PersistedItemsList>(m_cachedActuatorStatusFilenames);
}

const std::vector<std::string>& JsonPersistService::getPersistedSensorReadingFilenames(bool forceCacheReload)
{
    if (forceCacheReload || std::get<PersistedItemsListIsDirty>(m_cachedSensorReadingFilenames))
    {
        std::vector<std::string> sensorReadings;
        for (const std::string& sensorReading :
             FileSystemUtils::listFiles(PersistService::getPersistPath() + SENSOR_READING_DIRECTORY))
        {
            sensorReadings.emplace_back(PersistService::getPersistPath() + SENSOR_READING_DIRECTORY + sensorReading);
        }

        std::sort(sensorReadings.begin(), sensorReadings.end(),
                  [](const std::string& firstFilename, const std::string& secondFilename) -> bool {
                      try
                      {
                          auto a = std::stoull(StringUtils::tokenize(firstFilename, "/").back());
                          auto b = std::stoull(StringUtils::tokenize(secondFilename, "/").back());

                          return a < b;
                      }
                      catch (std::invalid_argument&)
                      {
                          return false;
                      }
                  });

        std::get<PersistedItemsList>(m_cachedSensorReadingFilenames) = std::move(sensorReadings);
        std::get<PersistedItemsListIsDirty>(m_cachedSensorReadingFilenames) = false;
    }

    return std::get<PersistedItemsList>(m_cachedSensorReadingFilenames);
}

const std::vector<std::string>& JsonPersistService::getPersistedAlarmFilenames(bool forceCacheReload)
{
    if (forceCacheReload || std::get<PersistedItemsListIsDirty>(m_cachedAlarmFilenames))
    {
        std::vector<std::string> alarms;
        for (const std::string& alarm : FileSystemUtils::listFiles(PersistService::getPersistPath() + ALARM_DIRECTORY))
        {
            alarms.emplace_back(PersistService::getPersistPath() + ALARM_DIRECTORY + alarm);
        }

        std::sort(alarms.begin(), alarms.end(),
                  [](const std::string& firstFilename, const std::string& secondFilename) -> bool {
                      try
                      {
                          auto a = std::stoull(StringUtils::tokenize(firstFilename, "/").back());
                          auto b = std::stoull(StringUtils::tokenize(secondFilename, "/").back());

                          return a < b;
                      }
                      catch (std::invalid_argument&)
                      {
                          return false;
                      }
                  });

        std::get<PersistedItemsList>(m_cachedAlarmFilenames) = std::move(alarms);
        std::get<PersistedItemsListIsDirty>(m_cachedAlarmFilenames) = false;
    }

    return std::get<PersistedItemsList>(m_cachedAlarmFilenames);
}

std::vector<std::string> JsonPersistService::getPersistedItemsFilenames(bool forceCacheReload)
{
    std::vector<std::string> files;
    auto alarms = getPersistedAlarmFilenames(forceCacheReload);
    files.insert(files.end(), alarms.begin(), alarms.end());

    auto actuatorStatuses = getPersistedActuatorStatusFilenames(forceCacheReload);
    files.insert(files.end(), actuatorStatuses.begin(), actuatorStatuses.end());

    auto sensorReadings = getPersistedSensorReadingFilenames(forceCacheReload);
    files.insert(files.end(), sensorReadings.begin(), sensorReadings.end());

    return files;
}

void JsonPersistService::invalidatePersistedItemsCache()
{
    std::get<PersistedItemsListIsDirty>(m_cachedActuatorStatusFilenames) = true;
    std::get<PersistedItemsListIsDirty>(m_cachedSensorReadingFilenames) = true;
    std::get<PersistedItemsListIsDirty>(m_cachedAlarmFilenames) = true;
}
}
