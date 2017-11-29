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

#include "connectivity/mqtt/JsonDtoParser.h"
#include "utilities/json.hpp"

#include "model/ActuatorCommand.h"
#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/SensorReading.h"

#include <string>

namespace wolkabout
{
using nlohmann::json;

/*** ACTUATOR COMMAND ***/
void to_json(json& j, const ActuatorCommand& p)
{
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

    p = ActuatorCommand(type == "SET" ? ActuatorCommand::Type::SET : ActuatorCommand::Type::STATUS, "", value);
}

bool JsonParser::fromJson(const std::string& jsonString, ActuatorCommand& actuatorCommandDto)
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
}
