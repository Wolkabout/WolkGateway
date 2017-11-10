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
JsonPersistService::JsonPersistService(std::string persistPath) : PersistService(std::move(persistPath))
{
    FileSystemUtils::createDirectory(PersistService::getPersistPath());

    getPersistedReadingsList(true);
}

bool JsonPersistService::hasPersistedReadings()
{
    return !getPersistedReadingsList().empty();
}

void JsonPersistService::persist(std::shared_ptr<Reading> reading)
{
    std::string name = PersistService::getPersistPath() + generateFileName(reading);
    std::string content = JsonPersistServiceParser::toJson(*reading);

    FileSystemUtils::createFileWithContent(name, content);
    invalidateCachedReadingsList();
}

std::shared_ptr<Reading> JsonPersistService::unpersistFirst()
{
    const std::vector<std::string>& readings = getPersistedReadingsList();
    const std::string& firstReading = readings.front();

    std::string readingJson;
    if (!FileSystemUtils::readFileContent(PersistService::getPersistPath() + firstReading, readingJson))
    {
        return nullptr;
    }

    if (StringUtils::endsWith(firstReading, ACTUATOR_STATUS_SUFFIX))
    {
        ActuatorStatus actuatorStatus;
        if (!JsonPersistServiceParser::fromJson(readingJson, actuatorStatus))
        {
            return nullptr;
        }

        return std::make_shared<ActuatorStatus>(actuatorStatus);
    }
    else if (StringUtils::endsWith(firstReading, ALARM_SUFFIX))
    {
        Alarm alarm;
        if (!JsonPersistServiceParser::fromJson(readingJson, alarm))
        {
            return nullptr;
        }

        return std::make_shared<Alarm>(alarm);
    }
    else if (StringUtils::endsWith(firstReading, SENSOR_READING_SUFFIX))
    {
        SensorReading sensorReading;
        if (!JsonPersistServiceParser::fromJson(readingJson, sensorReading))
        {
            return nullptr;
        }

        return std::make_shared<SensorReading>(sensorReading);
    }

    return nullptr;
}

void JsonPersistService::dropFirst()
{
    const std::vector<std::string>& readings = getPersistedReadingsList();
    FileSystemUtils::deleteFile(PersistService::getPersistPath() + readings.front());

    invalidateCachedReadingsList();
}

std::string JsonPersistService::generateFileName(std::shared_ptr<Reading> reading)
{
    struct ReadingPostfixGeneratorVisitor final : public ReadingVisitor
    {
        ReadingPostfixGeneratorVisitor(JsonPersistService& jjsonPersistService, Reading& rreading)
        : jsonPersistService(jjsonPersistService), reading(rreading), filename("")
        {
        }
        ~ReadingPostfixGeneratorVisitor() = default;

        void visit(ActuatorStatus& actuatorStatus) override
        {
            filename = actuatorStatus.getReference() + ACTUATOR_STATUS_SUFFIX;
        }

        void visit(Alarm&) override
        {
            filename = std::to_string(jsonPersistService.getLastPersistedReadingNumber() + 1) + ALARM_SUFFIX;
        }

        void visit(SensorReading&) override
        {
            filename = std::to_string(jsonPersistService.getLastPersistedReadingNumber() + 1) + SENSOR_READING_SUFFIX;
        }

        JsonPersistService& jsonPersistService;
        Reading& reading;
        std::string filename;
    } readingPostfixGeneratorVisitor = {*this, *reading};
    reading->acceptVisit(readingPostfixGeneratorVisitor);

    return readingPostfixGeneratorVisitor.filename;
}

unsigned long long int JsonPersistService::getLastPersistedReadingNumber()
{
    return getPersistedReadingsList().size();
}

const std::vector<std::string>& JsonPersistService::getPersistedReadingsList(bool ignoreCached)
{
    if (ignoreCached || std::get<CachedReadingsListIsDirty>(m_cachedReadingsList))
    {
        std::vector<std::string> readings = FileSystemUtils::listFiles(PersistService::getPersistPath());
        std::sort(readings.begin(), readings.end());

        std::get<CachedReadingsList>(m_cachedReadingsList) = readings;
        std::get<CachedReadingsListIsDirty>(m_cachedReadingsList) = false;
    }

    return std::get<CachedReadingsList>(m_cachedReadingsList);
}

void JsonPersistService::invalidateCachedReadingsList()
{
    std::get<CachedReadingsListIsDirty>(m_cachedReadingsList) = true;
}
}
