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

#ifndef CHANNELS_H
#define CHANNELS_H

#include <string>

namespace wolkabout
{
class Channel
{
public:
	Channel() = delete;

	static const std::string CHANNEL_DELIMITER;
	static const std::string CHANNEL_WILDCARD;

	static const std::string DEVICE_TO_PLATFORM_DIRECTION;
	static const std::string PLATFORM_TO_DEVICE_DIRECTION;

	static const std::string GATEWAY_PATH_PREFIX;
	static const std::string DEVICE_PATH_PREFIX;
	static const std::string REFERENCE_PATH_PREFIX;

	static const std::string SENSOR_READING_TYPE;
	static const std::string EVENT_TYPE;
	static const std::string ACTUATION_STATUS_TYPE;
	static const std::string ACTUATION_SET_TYPE;
	static const std::string ACTUATION_GET_TYPE;
	static const std::string CONFIGURATION_SET_TYPE;
	static const std::string CONFIGURATION_GET_TYPE;
	static const std::string DEVICE_STATUS_TYPE;
	static const std::string REGISTER_DEVICE_TYPE;
	static const std::string REREGISTER_DEVICE_TYPE;

	static const std::string SENSOR_READING_TOPIC_ROOT;
	static const std::string EVENTS_TOPIC_ROOT;
	static const std::string ACTUATION_STATUS_TOPIC_ROOT;
	static const std::string CONFIGURATION_SET_RESPONSE_TOPIC_ROOT;
	static const std::string CONFIGURATION_GET_RESPONSE_TOPIC_ROOT;
	static const std::string DEVICE_STATUS_TOPIC_ROOT;
	static const std::string DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT;
	static const std::string DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT;

	static const std::string ACTUATION_SET_TOPIC_ROOT;
	static const std::string ACTUATION_GET_TOPIC_ROOT;
	static const std::string CONFIGURATION_SET_REQUEST_TOPIC_ROOT;
	static const std::string CONFIGURATION_GET_REQUEST_TOPIC_ROOT;
	static const std::string DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT;
	static const std::string DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT;
};
}

#endif
