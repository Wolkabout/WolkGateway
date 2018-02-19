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

#ifndef MESSAGEFACTORY_H
#define MESSAGEFACTORY_H

#include "model/Message.h"

#include <memory>
#include <vector>

namespace wolkabout
{
class SensorReading;
class Alarm;
class ActuatorStatus;
class ActuatorSetCommand;
class ActuatorGetCommand;

class MessageFactory
{
public:
	virtual ~MessageFactory() = default;

	virtual std::shared_ptr<Message> make(const std::string& path,
										  std::vector<std::shared_ptr<SensorReading>> sensorReadings) = 0;
	virtual std::shared_ptr<Message> make(const std::string& path,
										  std::vector<std::shared_ptr<Alarm>> alarms) = 0;
	virtual std::shared_ptr<Message> make(const std::string& path,
										  std::vector<std::shared_ptr<ActuatorStatus>> actuatorStatuses) = 0;
	virtual std::shared_ptr<Message> make(const std::string& path,
										  std::shared_ptr<ActuatorSetCommand> command) = 0;
	virtual std::shared_ptr<Message> make(const std::string& path,
										  std::shared_ptr<ActuatorGetCommand> command) = 0;

	virtual std::shared_ptr<Message> make(const std::string& path,
										  const std::string& value) = 0;

	virtual bool fromJson(const std::string& jsonString, ActuatorSetCommand& command) = 0;
};
}

#endif
