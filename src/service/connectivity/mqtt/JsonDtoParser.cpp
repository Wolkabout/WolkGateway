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

#include "service/connectivity/mqtt/JsonDtoParser.h"
#include "service/connectivity/mqtt/dto/ActuatorCommandDto.h"
#include "service/connectivity/mqtt/dto/ActuatorStatusDto.h"
#include "service/connectivity/mqtt/dto/SensorReadingDto.h"
#include "utilities/json.hpp"

#include <string>

namespace wolkabout
{
using nlohmann::json;

/*** ACTUATOR COMMAND ***/
void to_json(json& j, const ActuatorCommandDto& p)
{
    j = json{{"command", p.getType() == ActuatorCommand::Type::SET ? "SET" : "STATUS"}, {"value", p.getValue()}};
}

void from_json(const json& j, ActuatorCommandDto& p)
{
    const std::string type = j.at("command").get<std::string>();
    const std::string value = [&]() -> std::string {
        if (j.find("value") != j.end())
        {
            return j.at("value").get<std::string>();
        }
        else
        {
            return "";
        }
    }();

    p = ActuatorCommandDto(type == "SET" ? ActuatorCommand::Type::SET : ActuatorCommand::Type::STATUS, value);
}

std::string JsonDtoParser::toJson(ActuatorCommandDto& actutorCommandDto)
{
    json j = actutorCommandDto;
    return j.dump();
}

bool JsonDtoParser::fromJson(const std::string& jsonString, ActuatorCommandDto& actuatorCommandDto)
{
    try
    {
        json j = json::parse(jsonString);
        actuatorCommandDto = j;
    }
    catch (...)
    {
        return false;
    }

    return true;
}
/*** ACTUATOR COMMAND ***/

/*** ACTUATOR STATUS ***/
void to_json(json& j, const ActuatorStatusDto& p)
{
    const std::string status = [&]() -> std::string {
        if (p.getState() == ActuatorStatus::State::READY)
        {
            return "READY";
        }
        else if (p.getState() == ActuatorStatus::State::BUSY)
        {
            return "BUSY";
        }
        else if (p.getState() == ActuatorStatus::State::ERROR)
        {
            return "ERROR";
        }

        return "ERROR";
    }();

    j = json{{"status", status}, {"value", p.getValue()}};
}

void from_json(const json& j, ActuatorStatusDto& p)
{
    std::string st = j.at("status").get<std::string>();
    std::string value = j.at("value").get<std::string>();

    const ActuatorStatus::State state = [&]() -> ActuatorStatus::State {
        if (st == "READY")
        {
            return ActuatorStatus::State::READY;
        }
        else if (st == "BUSY")
        {
            return ActuatorStatus::State::BUSY;
        }
        else if (st == "ERROR")
        {
            return ActuatorStatus::State::ERROR;
        }

        return ActuatorStatus::State::ERROR;
    }();

    p = ActuatorStatusDto(state, value);
}

std::string JsonDtoParser::toJson(const ActuatorStatusDto& actuatorStatusDto)
{
    json j = actuatorStatusDto;
    return j.dump();
}

bool JsonDtoParser::fromJson(const std::string& jsonString, ActuatorStatusDto& actuatorStatusDto)
{
    try
    {
        json j = json::parse(jsonString);
        actuatorStatusDto = j;
    }
    catch (...)
    {
        return false;
    }

    return true;
}
/*** ACTUATOR STATUS ***/

/*** ALARM ***/
void to_json(json& j, const AlarmDto& p)
{
    if (p.getRtc() == 0)
    {
        j = json{{"data", p.getValue()}};
    }
    else
    {
        j = json{{"utc", p.getRtc()}, {"data", p.getValue()}};
    }
}

void from_json(const json& j, AlarmDto& p)
{
    auto rtc = j.at("utc").get<unsigned long long int>();
    std::string value = j.at("data").get<std::string>();

    p = AlarmDto(rtc, value);
}

std::string JsonDtoParser::toJson(const AlarmDto& alarmDto)
{
    json j = alarmDto;
    return j.dump();
}

bool JsonDtoParser::fromJson(const std::string& jsonString, AlarmDto& alarmDto)
{
    try
    {
        json j = json::parse(jsonString);
        alarmDto = j;
    }
    catch (...)
    {
        return false;
    }

    return true;
}
/*** ALARM ***/

/*** SENSOR READING ***/
void to_json(json& j, const SensorReadingDto& p)
{
    if (p.getRtc() == 0)
    {
        j = json{{"data", p.getValue()}};
    }
    else
    {
        j = json{{"utc", p.getRtc()}, {"data", p.getValue()}};
    }
}

void from_json(const json& j, SensorReadingDto& p)
{
    auto rtc = j.at("utc").get<unsigned long long int>();
    std::string value = j.at("data").get<std::string>();

    p = SensorReadingDto(rtc, value);
}

std::string JsonDtoParser::toJson(const SensorReadingDto& sensorReadingDto)
{
    json j = sensorReadingDto;
    return j.dump();
}

bool JsonDtoParser::fromJson(const std::string& jsonString, SensorReadingDto& sensorReadingDto)
{
    try
    {
        json j = json::parse(jsonString);
        sensorReadingDto = j;
    }
    catch (...)
    {
        return false;
    }

    return true;
}
/*** SENSOR READING ***/
}
