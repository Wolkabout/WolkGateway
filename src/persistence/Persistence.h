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

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/SensorReading.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
/**
 * @brief A storage designed for holding elements in persistent store prior to publishing to WolkAbout IoT Platform.
 *
 * Multiple Readings can be stored under the same key.
 * Multiple Alarms can be stored under the same key.
 * Single ActuatorStatus can be stored under one key.
 *
 * Implementation storing/retrieving strategy must be FIFO.
 */
class Persistence
{
public:
    /**
     * @brief Destructor
     */
    virtual ~Persistence() = default;

    /**
     * @brief Inserts the wolkabout::SensorReading
     *
     * @param key     with which wolkabout::SensorReading should be associated
     * @param reading to be inserted
     * @return {@code true} if successful, or {@code false} if
     * element can not be inserted
     */
    virtual bool putSensorReading(const std::string& key, std::shared_ptr<SensorReading> sensorReading) = 0;

    /**
     * @brief Retrieves, first {@code count} wolkabout::SensorReadings of this storage, associated with given {@code
     * key} or returns empty {@code std::vector<std::shared_ptr<SensorReading>>} if this storage is empty.
     *
     * @param key   of the wolkabout::SensorReadings
     * @param count number of items to peek
     * @return {@code std::vector<std::shared_ptr<SensorReading>>} containing {@code count} wolkabout::SensorReadings
     * starting from the head, or returns less than {@code count} wolkabout::SensorReadings if this storage does not
     * have requested number of elements
     */
    virtual std::vector<std::shared_ptr<SensorReading>> getSensorReadings(const std::string& key,
																		  std::uint_fast64_t count) = 0;

    /**
     * @brief Removes first {@code count} wolkabout::SensorReadings of this storage, associated with given {@code key}.
     *
     * @param key   of the wolkabout::SensorReadings
     * @param count number of items to remove
     */
	virtual void removeSensorReadings(const std::string& key, std::uint_fast64_t count) = 0;

    /**
     * Returns {@code std::vector<std::string>>} of wolkabout::SensorReadings keys contained in this storage.
     *
     * @return {@code std::vector<std::string>} containing keys, or empty {@code std::vector<std::string>>} if no
     * wolkabout::SensorReadings are present.
     */
    virtual std::vector<std::string> getSensorReadingsKeys() = 0;

    /**
     * @brief Inserts the wolkabout::Alarm
     *
     * @param key     with which wolkabout::Alarm should be associated
     * @param reading to be inserted
     * @return {@code true} if successful, or {@code false} if
     * element can not be inserted
     */
    virtual bool putAlarm(const std::string& key, std::shared_ptr<Alarm> alarm) = 0;

    /**
     * @brief Retrieves, first {@code count} wolkabout::SensorReadings of this storage, associated with given {@code
     * key} or returns empty {@code std::vector<std::shared_ptr<SensorReading>>} if this storage is empty.
     *
     * @param key   of the wolkabout::SensorReadings
     * @param count number of items to peek
     * @return {@code std::vector<std::shared_ptr<SensorReading>>} containing {@code count} wolkabout::SensorReadings
     * starting from the head, or returns less than {@code count} wolkabout::SensorReadings if this storage does not
     * have requested number of elements
     */
	virtual std::vector<std::shared_ptr<Alarm>> getAlarms(const std::string& key, std::uint_fast64_t count) = 0;

    /**
     * @brief Removes first {@code count} wolkabout::Alarms of this storage, associated with given {@code key}.
     *
     * @param key   of the wolkabout::Alarms
     * @param count number of items to remove
     */
	virtual void removeAlarms(const std::string& key, std::uint_fast64_t count) = 0;

    /**
     * @brief Returns {@code std::vector<std::string>>} of wolkabout::Alarm keys contained in this storage
     *
     * @return {@code std::vector<std::string>>} containing keys, or empty {@code std::vector<std::string>>} if no
     * wolkabout::Alarms are present.
     */
    virtual std::vector<std::string> getAlarmsKeys() = 0;

    /**
     * @brief Inserts the wolkabout::ActuatorStatus.
     *
     * @param key            with which wolkabout::ActuatorStatus should be associated.
     * @param actuatorStatus to be inserted
     * @return {@code true} if successful, or {@code false} if
     * element can not be inserted
     */
    virtual bool putActuatorStatus(const std::string& key, std::shared_ptr<ActuatorStatus> actuatorStatus) = 0;

    /**
     * @brief Retrieves, wolkabout::ActuatorStatus of this storage, associated with given {@code key}.
     *
     * @param key of the wolkabout::ActuatorStatus.
     * @return {@code std::shared_ptr<wolkabout::ActuatorStatus>} for given {@code key}
     */
    virtual std::shared_ptr<ActuatorStatus> getActuatorStatus(const std::string& key) = 0;

    /**
     * @brief Removes wolkabout::ActuatorStatus from this storage, associated with given {@code key}.
     *
     * @param key of the wolkabout::Reading
     */
    virtual void removeActuatorStatus(const std::string& key) = 0;

    /**
     * Returns {@code std::vector<std::string>} of wolkabout::ActuatorStatus keys contained in this storage.
     *
     * @return {@code std::vector<std::string>} containing keys, or empty {@code std::vector<std::string>} if no
     * wolkabout::ActuatorStatuses are present.
     */
    virtual std::vector<std::string> getGetActuatorStatusesKeys() = 0;

    /**
     * Returns {@code true} if this storage contains no wolkabout::SensorReadings, wolkabout::ActuatorStatuses and
     * wolkabout::Alarms associated with any key.
     *
     * @return {@code true} if this storage contains no wolkabout::SensorReadings, wolkabout::ActuatorStatuses and
     * wolkabout::Alarms associated with any key
     */
    virtual bool isEmpty() = 0;
};
}

#endif
