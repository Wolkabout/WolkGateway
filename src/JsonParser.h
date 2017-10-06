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

#ifndef JSONPARSER_H
#define JSONPARSER_H

#include "ActuatorCommand.h"
#include "ActuatorStatus.h"
#include "Alarm.h"
#include "SensorReading.h"

#include <string>

namespace wolkabout
{
class JsonParser
{
    JsonParser() = delete;

public:
    static std::string toJson(ActuatorCommand actutorCommand);
    static void fromJson(std::string jsonString, ActuatorCommand& actuatorCommand);

    static std::string toJson(ActuatorStatus actuatorStatus);
    static void fromJson(std::string jsonString, ActuatorStatus& actuatorStatus);

    static std::string toJson(Alarm event);
    static void fromJson(std::string jsonString, Alarm& event);

    static std::string toJson(SensorReading sensorReading);
    static void fromJson(std::string jsonString, SensorReading& sensorReading);
};
}

#endif
