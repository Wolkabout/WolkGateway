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

#include "service/persist/json/JsonPersistServiceParser.h"
#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/SensorReading.h"
#include "utilities/json.hpp"

#include <string>

namespace wolkabout
{
using nlohmann::json;

/*** ACTUATOR STATUS ***/
void to_json(json& j, const ActuatorStatus& p)
{
    const std::string state = [&]() -> std::string {
        if (p.getState() == ActuatorStatus::State::READY)
        {
            return "R";
        }
        else if (p.getState() == ActuatorStatus::State::BUSY)
        {
            return "B";
        }
        else if (p.getState() == ActuatorStatus::State::ERROR)
        {
            return "E";
        }

        return "E";
    }();

    j = json{{"rtc", p.getRtc()}, {"ref", p.getReference()}, {"val", p.getValue()}, {"state", state}};
}

void from_json(const json& j, ActuatorStatus& p)
{
    const std::string reference = j.at("ref").get<std::string>();
    const std::string value = j.at("val").get<std::string>();
    std::string st = j.at("state").get<std::string>();

    const ActuatorStatus::State state = [&]() -> ActuatorStatus::State {
        if (st == "R")
        {
            return ActuatorStatus::State::READY;
        }
        else if (st == "B")
        {
            return ActuatorStatus::State::BUSY;
        }
        else if (st == "E")
        {
            return ActuatorStatus::State::ERROR;
        }

        return ActuatorStatus::State::ERROR;
    }();

    p = ActuatorStatus(value, reference, state);
}

std::string JsonPersistServiceParser::toJson(const ActuatorStatus& actuatorStatus)
{
    json j = actuatorStatus;
    return j.dump();
}

bool JsonPersistServiceParser::fromJson(const std::string& jsonString, ActuatorStatus& actuatorStatus)
{
    try
    {
        json j = json::parse(jsonString);
        actuatorStatus = j;
    }
    catch (...)
    {
        return false;
    }

    return true;
}
/*** ACTUATOR STATUS ***/

/*** ALARM ***/
void to_json(json& j, const Alarm& p)
{
    std::string command;

    j = json{{"rtc", p.getRtc()}, {"ref", p.getReference()}, {"val", p.getValue()}};
}

void from_json(const json& j, Alarm& p)
{
    const auto rtc = j.at("rtc").get<unsigned long long int>();
    const std::string reference = j.at("ref").get<std::string>();
    const std::string value = j.at("val").get<std::string>();

    p = Alarm(value, reference, rtc);
}

std::string JsonPersistServiceParser::toJson(const Alarm& alarm)
{
    json j = alarm;
    return j.dump();
}

bool JsonPersistServiceParser::fromJson(const std::string& jsonString, Alarm& alarm)
{
    try
    {
        json j = json::parse(jsonString);
        alarm = j;
    }
    catch (...)
    {
        return false;
    }

    return true;
}
/*** ALARM ***/

/*** SENSOR READING ***/
void to_json(json& j, const SensorReading& p)
{
    std::string command;

    j = json{{"rtc", p.getRtc()}, {"ref", p.getReference()}, {"val", p.getValue()}};
}

void from_json(const json& j, SensorReading& p)
{
    const auto rtc = j.at("rtc").get<unsigned long long int>();
    const std::string reference = j.at("ref").get<std::string>();
    const std::string value = j.at("val").get<std::string>();

    p = SensorReading(value, reference, rtc);
}

std::string JsonPersistServiceParser::toJson(const SensorReading& sensorReading)
{
    json j = sensorReading;
    return j.dump();
}

bool JsonPersistServiceParser::fromJson(const std::string& jsonString, SensorReading& sensorReading)
{
    try
    {
        json j = json::parse(jsonString);
        sensorReading = j;
    }
    catch (...)
    {
        return false;
    }

    return true;
}

std::string JsonPersistServiceParser::toJson(Reading& reading)
{
    std::string result;
    struct ReadingSerializerVisitor final : public ReadingVisitor
    {
        ReadingSerializerVisitor(std::string& result) : m_result(result) {}
        ~ReadingSerializerVisitor() = default;

        void visit(ActuatorStatus& actuatorStatus) override
        {
            m_result = JsonPersistServiceParser::toJson(actuatorStatus);
        }

        void visit(Alarm& alarm) override { m_result = JsonPersistServiceParser::toJson(alarm); }

        void visit(SensorReading& sensorReading) override
        {
            m_result = JsonPersistServiceParser::toJson(sensorReading);
        }

        std::string& m_result;
    } readingSerializerVisitor = {result};
    reading.acceptVisit(readingSerializerVisitor);

    return result;
}
/*** SENSOR READING ***/
}
