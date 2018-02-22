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

#include "Channels.h"

namespace wolkabout
{
const std::string Channel::CHANNEL_DELIMITER = "/";
const std::string Channel::CHANNEL_WILDCARD = "#";

const std::string Channel::DEVICE_TO_PLATFORM_DIRECTION = "d2p";
const std::string Channel::PLATFORM_TO_DEVICE_DIRECTION = "p2d";

const std::string Channel::GATEWAY_PATH_PREFIX = std::string{"g"} + CHANNEL_DELIMITER;
const std::string Channel::DEVICE_PATH_PREFIX = std::string{"d"} + CHANNEL_DELIMITER;

const std::string Channel::SENSOR_READING_TYPE = "sensor_reading";
const std::string Channel::EVENT_TYPE = "events";
const std::string Channel::ACTUATION_STATUS_TYPE = "actuator_status";
const std::string Channel::ACTUATION_SET_TYPE = "actuator_set";
const std::string Channel::ACTUATION_GET_TYPE = "actuator_get";
const std::string Channel::CONFIGURATION_SET_TYPE = "configuration_set";
const std::string Channel::CONFIGURATION_GET_TYPE = "configuration_get";
const std::string Channel::DEVICE_STATUS_TYPE = "status";
const std::string Channel::REGISTER_DEVICE_TYPE = "register_device";
const std::string Channel::REREGISTER_DEVICE_TYPE = "reregister_device";

const std::string Channel::SENSOR_READING_TOPIC_ROOT =
  DEVICE_TO_PLATFORM_DIRECTION + CHANNEL_DELIMITER + SENSOR_READING_TYPE + CHANNEL_DELIMITER;
const std::string Channel::EVENTS_TOPIC_ROOT =
  DEVICE_TO_PLATFORM_DIRECTION + CHANNEL_DELIMITER + EVENT_TYPE + CHANNEL_DELIMITER;
const std::string Channel::ACTUATION_STATUS_TOPIC_ROOT =
  DEVICE_TO_PLATFORM_DIRECTION + CHANNEL_DELIMITER + ACTUATION_STATUS_TYPE + CHANNEL_DELIMITER;
const std::string Channel::CONFIGURATION_GET_RESPONSE_TOPIC_ROOT =
  DEVICE_TO_PLATFORM_DIRECTION + CHANNEL_DELIMITER + CONFIGURATION_GET_TYPE + CHANNEL_DELIMITER;
const std::string Channel::CONFIGURATION_SET_RESPONSE_TOPIC_ROOT =
  DEVICE_TO_PLATFORM_DIRECTION + CHANNEL_DELIMITER + CONFIGURATION_SET_TYPE + CHANNEL_DELIMITER;
const std::string Channel::DEVICE_STATUS_TOPIC_ROOT =
  DEVICE_TO_PLATFORM_DIRECTION + CHANNEL_DELIMITER + DEVICE_STATUS_TYPE + CHANNEL_DELIMITER;
const std::string Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT =
  DEVICE_TO_PLATFORM_DIRECTION + CHANNEL_DELIMITER + REGISTER_DEVICE_TYPE + CHANNEL_DELIMITER;
const std::string Channel::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT =
  DEVICE_TO_PLATFORM_DIRECTION + CHANNEL_DELIMITER + REREGISTER_DEVICE_TYPE + CHANNEL_DELIMITER;

const std::string Channel::ACTUATION_SET_TOPIC_ROOT =
  PLATFORM_TO_DEVICE_DIRECTION + CHANNEL_DELIMITER + ACTUATION_SET_TYPE + CHANNEL_DELIMITER;
const std::string Channel::ACTUATION_GET_TOPIC_ROOT =
  PLATFORM_TO_DEVICE_DIRECTION + CHANNEL_DELIMITER + ACTUATION_GET_TYPE + CHANNEL_DELIMITER;
const std::string Channel::CONFIGURATION_SET_REQUEST_TOPIC_ROOT =
  PLATFORM_TO_DEVICE_DIRECTION + CHANNEL_DELIMITER + CONFIGURATION_SET_TYPE + CHANNEL_DELIMITER;
const std::string Channel::CONFIGURATION_GET_REQUEST_TOPIC_ROOT =
  PLATFORM_TO_DEVICE_DIRECTION + CHANNEL_DELIMITER + CONFIGURATION_GET_TYPE + CHANNEL_DELIMITER;
const std::string Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT =
  PLATFORM_TO_DEVICE_DIRECTION + CHANNEL_DELIMITER + REGISTER_DEVICE_TYPE + CHANNEL_DELIMITER;
const std::string Channel::DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT =
  PLATFORM_TO_DEVICE_DIRECTION + CHANNEL_DELIMITER + REREGISTER_DEVICE_TYPE + CHANNEL_DELIMITER;
}    // namespace wolkabout
