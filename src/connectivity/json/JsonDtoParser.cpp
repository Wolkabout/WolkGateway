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

#include "connectivity/json/JsonDtoParser.h"
#include "utilities/json.hpp"

#include "model/ActuatorCommand.h"
#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/SensorReading.h"
#include "model/FirmwareUpdateCommand.h"
#include "utilities/StringUtils.h"

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

		return "";
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

/*** FIRMWARE UPDATE COMMAND ***/
void from_json(const json& j, FirmwareUpdateCommand& p)
{
	const std::string typeStr = j.at("command").get<std::string>();

	FirmwareUpdateCommand::Type type;
	if(typeStr == "INSTALL")
	{
		type = FirmwareUpdateCommand::Type::INSTALL;
	}
	else if(typeStr == "ABORT")
	{
		type = FirmwareUpdateCommand::Type::ABORT;
	}
	else if(typeStr == "FILE_UPLOAD")
	{
		type = FirmwareUpdateCommand::Type::FILE_UPLOAD;
	}
	else if(typeStr == "URL_DOWNLOAD")
	{
		type = FirmwareUpdateCommand::Type::URL_DOWNLOAD;
	}
	else
	{
		type = FirmwareUpdateCommand::Type::UNKNOWN;
	}

	const bool autoInstall = [&]() -> bool {
		if (j.find("autoInstall") != j.end())
		{
			return j.at("autoInstall").get<bool>();
		}

		return false;
	}();

	if(type == FirmwareUpdateCommand::Type::FILE_UPLOAD)
	{
		const std::string name = [&]() -> std::string {
			if (j.find("fileName") != j.end())
			{
				return j.at("fileName").get<std::string>();
			}

			return "";
		}();

		const uint_fast64_t size = [&]() -> uint_fast64_t {
			if (j.find("fileSize") != j.end())
			{
				return j.at("fileSize").get<uint_fast64_t>();
			}

			return 0;
		}();

		const std::string hash = [&]() -> std::string {
			if (j.find("fileHash") != j.end())
			{
				return j.at("fileHash").get<std::string>();
			}

			return "";
		}();

		p = FirmwareUpdateCommand(type, name, size, hash, autoInstall);
		return;
	}
	else if(type == FirmwareUpdateCommand::Type::URL_DOWNLOAD)
	{
		const std::string url = [&]() -> std::string {
			if (j.find("fileUrl") != j.end())
			{
				return j.at("fileUrl").get<std::string>();
			}

			return "";
		}();

		p = FirmwareUpdateCommand(type, url, autoInstall);
		return;
	}
	else
	{
		p = FirmwareUpdateCommand(type);
	}
}

bool JsonParser::fromJson(const std::string& jsonString, FirmwareUpdateCommand& firmwareUpdateCommandDto)
{
	try
	{
		if(StringUtils::startsWith(jsonString, "{"))
		{
			json j = json::parse(jsonString);
			firmwareUpdateCommandDto = j;
		}
		else
		{
			FirmwareUpdateCommand::Type type;
			if(jsonString == "INSTALL")
			{
				type = FirmwareUpdateCommand::Type::INSTALL;
			}
			else if(jsonString == "ABORT")
			{
				type = FirmwareUpdateCommand::Type::ABORT;
			}
			else if(jsonString == "FILE_UPLOAD")
			{
				type = FirmwareUpdateCommand::Type::FILE_UPLOAD;
			}
			else if(jsonString == "URL_DOWNLOAD")
			{
				type = FirmwareUpdateCommand::Type::URL_DOWNLOAD;
			}
			else
			{
				type = FirmwareUpdateCommand::Type::UNKNOWN;
			}

			firmwareUpdateCommandDto = FirmwareUpdateCommand(type);
		}
	}
	catch (...)
	{
		return false;
	}

	return true;
}
/*** FIRMWARE UPDATE COMMAND ***/
}
