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

#include "JsonMessageFactory.h"
#include "utilities/json.hpp"
#include "utilities/Logger.h"
#include "model/ActuatorSetCommand.h"
#include "model/ActuatorGetCommand.h"
#include "model/SensorReading.h"
#include "model/Alarm.h"
#include "model/ActuatorStatus.h"

using nlohmann::json;

namespace wolkabout
{
void from_json(const json& j, SensorReading& reading)
{
	const std::string value = [&]() -> std::string {
		if (j.find("value") != j.end())
		{
			return j.at("value").get<std::string>();
		}

		return "";
	}();

	reading = SensorReading("", value);
}

void from_json(const json& j, ActuatorSetCommand& command)
{
	const std::string value = [&]() -> std::string {
		if (j.find("value") != j.end())
		{
			return j.at("value").get<std::string>();
		}

		return "";
	}();

	command = ActuatorSetCommand("", value);
}

//void from_json(const json& j, ActuatorGetCommand& command)
//{
//	command = ActuatorGetCommand("");
//}

std::shared_ptr<Message> JsonMessageFactory::make(const std::string& path,
												  std::vector<std::shared_ptr<SensorReading>> sensorReadings)
{

}

std::shared_ptr<Message> JsonMessageFactory::make(const std::string& path,
												  std::vector<std::shared_ptr<Alarm>> alarms)
{

}

std::shared_ptr<Message> JsonMessageFactory::make(const std::string& path,
												  std::vector<std::shared_ptr<ActuatorStatus>> actuatorStatuses)
{

}

std::shared_ptr<Message> JsonMessageFactory::make(const std::string& path,
												  std::shared_ptr<ActuatorSetCommand> command)
{

}

std::shared_ptr<Message> JsonMessageFactory::make(const std::string& path, std::shared_ptr<ActuatorGetCommand> command)
{

}

std::shared_ptr<Message> JsonMessageFactory::make(const std::string& path, const std::string& value)
{

}

//bool JsonMessageFactory::fromJson(const std::string& jsonString, SensorReading& reading)
//{
//	try
//	{
//		json j = json::parse(jsonString);
//		reading = j;
//	}
//	catch (...)
//	{
//		LOG(DEBUG) << "Unable to parse SensorReading: " << jsonString;
//		return false;
//	}

//	return true;
//}

//bool JsonMessageFactory::fromJson(const std::string& jsonString, Alarm& alarm)
//{
//	try
//	{
//		json j = json::parse(jsonString);
//		alarm = j;
//	}
//	catch (...)
//	{
//		LOG(DEBUG) << "Unable to parse Alarm: " << jsonString;
//		return false;
//	}

//	return true;
//}

//bool JsonMessageFactory::fromJson(const std::string& jsonString, ActuatorStatus& status)
//{
//	try
//	{
//		json j = json::parse(jsonString);
//		status = j;
//	}
//	catch (...)
//	{
//		LOG(DEBUG) << "Unable to parse ActuatorStatus: " << jsonString;
//		return false;
//	}

//	return true;
//}

bool JsonMessageFactory::fromJson(const std::string& jsonString, ActuatorSetCommand& command)
{
	try
	{
		json j = json::parse(jsonString);
		command = j;
	}
	catch (...)
	{
		LOG(DEBUG) << "Unable to parse ActuatorSetCommand: " << jsonString;
		return false;
	}

	return true;
}

//bool JsonMessageFactory::fromJson(const std::string& jsonString, ActuatorGetCommand& command)
//{
//	try
//	{
//		json j = json::parse(jsonString);
//		command = j;
//	}
//	catch (...)
//	{
//		LOG(DEBUG) << "Unable to parse ActuatorGetCommand: " << jsonString;
//		return false;
//	}

//	return true;
//}
}
