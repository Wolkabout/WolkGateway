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

#ifndef PERSISTSERVICE_H
#define PERSISTSERVICE_H

#include <memory>
#include <string>

namespace wolkabout
{
class Reading;
/**
 * @brief The PersistService class defines interface which custom persistence mechanism should implement
 */
class PersistService
{
public:
    /**
     * @brief Constructor
     * @param persistPath Relative, or absolute, path to directory to be used with persistence implementation
     */
    PersistService(std::string persistPath = "");

    /**
     * @brief Destructor
     */
    virtual ~PersistService() = default;

    /**
     * @brief Check whether persisted reading(s) exist
     * @return true if there are persisted readings
     */
    virtual bool hasPersistedReadings() = 0;

    /**
     * @brief Persists reading
     * @param reading to be persisted
     */
    virtual void persist(std::shared_ptr<Reading> reading) = 0;

    /**
     * @brief Unpersists reading
     * @return If reading is unpersisted successfully std::shared_ptr<Reading>, otherwise nullptr
     */
    virtual std::shared_ptr<Reading> unpersistFirst() = 0;

    /**
     * @brief Removes first reading from persistence
     */
    virtual void dropFirst() = 0;

    /**
     * @brief Returns path to directory used by persistence
     * @return çonst std::string& containing path to directory used by persistence
     */
    const std::string& getPersistPath() const;

private:
    std::string m_persistPath;
};
}

#endif
