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

#ifndef JSONDTOPARSER_H
#define JSONDTOPARSER_H

#include "service/connectivity/mqtt/dto/ActuatorCommandDto.h"
#include "service/connectivity/mqtt/dto/ActuatorStatusDto.h"
#include "service/connectivity/mqtt/dto/AlarmDto.h"
#include "service/connectivity/mqtt/dto/SensorReadingDto.h"

#include <string>

namespace wolkabout
{
class JsonDtoParser
{
public:
    JsonDtoParser() = delete;

    static std::string toJson(ActuatorCommandDto& actutorCommandDto);
    static bool fromJson(const std::string& jsonString, ActuatorCommandDto& actuatorCommandDto);

    static std::string toJson(const ActuatorStatusDto& actuatorStatusDto);
    static bool fromJson(const std::string& jsonString, ActuatorStatusDto& actuatorStatusDto);

    static std::string toJson(const AlarmDto& alarmDto);
    static bool fromJson(const std::string& jsonString, AlarmDto& alarmDto);

    static std::string toJson(const SensorReadingDto& sensorReadingDto);
    static bool fromJson(const std::string& jsonString, SensorReadingDto& sensorReadingDto);
};
}

#endif
