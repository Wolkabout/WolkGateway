/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#ifndef WOLKABOUT_DATAHANDLER_H
#define WOLKABOUT_DATAHANDLER_H

#include "core/model/ActuatorStatus.h"
#include "core/model/Alarm.h"
#include "core/model/ConfigurationItem.h"
#include "core/model/DeviceStatus.h"
#include "core/model/SensorReading.h"

#include <string>
#include <vector>

namespace wolkabout
{
class DataHandler
{
public:
    virtual ~DataHandler() = default;

    virtual void addSensorReading(const std::string& deviceKey, const SensorReading& reading) = 0;
    virtual void addSensorReadings(const std::string& deviceKey, const std::vector<SensorReading>& readings) = 0;

    virtual void addAlarm(const std::string& deviceKey, const Alarm& alarm) = 0;

    virtual void addActuatorStatus(const std::string& deviceKey, const ActuatorStatus& status) = 0;

    virtual void addConfiguration(const std::string& deviceKey,
                                  const std::vector<ConfigurationItem>& configurations) = 0;

    virtual void addDeviceStatus(const DeviceStatus& status) = 0;
};
}    // namespace wolkabout

#endif    // WOLKABOUT_DATAHANDLER_H
