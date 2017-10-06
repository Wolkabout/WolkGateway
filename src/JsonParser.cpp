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

#include "JsonParser.h"
#include "ActuatorCommand.h"
#include "ActuatorStatus.h"
#include "SensorReading.h"
#include "json/json.hpp"

#include <string>

using nlohmann::json;

namespace wolkabout
{
/*** ACTUATOR COMMAND ***/
void to_json(json& j, const ActuatorCommand& p)
{
    std::string command;

    j = json{{"command", p.getType() == ActuatorCommand::Type::SET ? "SET" : "STATUS"}, {"value", p.getValue()}};
}

void from_json(const json& j, ActuatorCommand& p)
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

    p = ActuatorCommand(type.compare("SET") == 0 ? ActuatorCommand::Type::SET : ActuatorCommand::Type::STATUS, value);
}

std::string JsonParser::toJson(ActuatorCommand actuatorCommand)
{
    json j = actuatorCommand;
    return j.dump();
}

void JsonParser::fromJson(std::string jsonString, ActuatorCommand& actuatorCommand)
{
    json j = json::parse(jsonString);
    actuatorCommand = j;
}
/*** ACTUATOR COMMAND ***/

/*** ACTUATOR STATUS ***/
void to_json(json& j, const ActuatorStatus& p)
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

void from_json(const json& j, ActuatorStatus& p)
{
    std::string st = j.at("status").get<std::string>();
    std::string value = j.at("value").get<std::string>();

    const ActuatorStatus::State state = [&]() -> ActuatorStatus::State {
        if (st.compare("READY") == 0)
        {
            return ActuatorStatus::State::READY;
        }
        else if (st.compare("BUSY") == 0)
        {
            return ActuatorStatus::State::BUSY;
        }
        else if (st.compare("ERROR") == 0)
        {
            return ActuatorStatus::State::ERROR;
        }

        return ActuatorStatus::State::ERROR;
    }();

    p = ActuatorStatus(value, state);
}

std::string JsonParser::toJson(ActuatorStatus actuatorStatus)
{
    json j = actuatorStatus;
    return j.dump();
}

void JsonParser::fromJson(std::string jsonString, ActuatorStatus& actuatorStatus)
{
    json j = json::parse(jsonString);
    actuatorStatus = j;
}
/*** ACTUATOR STATUS ***/

/*** ALARM ***/
void to_json(json& j, const Alarm& p)
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

std::string JsonParser::toJson(Alarm event)
{
    json j = event;
    return j.dump();
}

void from_json(const json& j, Alarm& p) {}

void JsonParser::fromJson(std::string jsonString, Alarm& event)
{
    json j = json::parse(jsonString);
    event = j;
}
/*** EVENT ***/

/*** SENSOR READING ***/
void to_json(json& j, const SensorReading& p)
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

void from_json(const json& j, SensorReading& p) {}

std::string JsonParser::toJson(SensorReading actutorCommand)
{
    json j = actutorCommand;
    return j.dump();
}

void JsonParser::fromJson(std::string jsonString, SensorReading& sensorReading)
{
    json j = json::parse(jsonString);
    sensorReading = j;
}
/*** SENSOR READING ***/
}
