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

#ifndef JSONPERSISTSERVICEPARSER_H
#define JSONPERSISTSERVICEPARSER_H

#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/Reading.h"
#include "model/SensorReading.h"
#include "utilities/json.hpp"

#include <string>

namespace wolkabout
{
class JsonPersistServiceParser
{
public:
    JsonPersistServiceParser() = delete;

    static std::string toJson(const ActuatorStatus& actuatorStatus);
    static bool fromJson(const std::string& jsonString, ActuatorStatus& actuatorStatus);

    static std::string toJson(const Alarm& alarm);
    static bool fromJson(const std::string& jsonString, Alarm& alarm);

    static std::string toJson(const SensorReading& sensorReading);
    static bool fromJson(const std::string& jsonString, SensorReading& sensorReading);

    static std::string toJson(Reading& reading);
};
}

#endif
