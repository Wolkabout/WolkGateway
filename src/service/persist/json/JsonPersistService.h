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

#include "model/Reading.h"
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
    JsonPersistService(std::string persistPath = "persistence");
    virtual ~JsonPersistService() = default;

    bool hasPersistedReadings() override;

    void persist(std::shared_ptr<Reading> reading) override;

    std::shared_ptr<Reading> unpersistFirst() override;
    void dropFirst() override;

private:
    std::string generateFileName(std::shared_ptr<Reading> reading);
    unsigned long long int getLastPersistedReadingNumber();

    const std::vector<std::string>& getPersistedReadingsList(bool ignoreCached = false);
    void invalidateCachedReadingsList();

    // std::pair<Is list dirty, Readings list>
    std::pair<bool, std::vector<std::string>> m_cachedReadingsList;
    static const constexpr int CachedReadingsListIsDirty = 0;
    static const constexpr int CachedReadingsList = 1;

    static const constexpr char* ACTUATOR_STATUS_SUFFIX = "-actuator_status";
    static const constexpr char* SENSOR_READING_SUFFIX = "-sensor-reading";
    static const constexpr char* ALARM_SUFFIX = "-alarm";
};
}

#endif
