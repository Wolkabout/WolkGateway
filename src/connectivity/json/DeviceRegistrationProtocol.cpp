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

#include "DeviceRegistrationProtocol.h"
#include "connectivity/Channels.h"
#include "model/DeviceRegistrationRequestDto.h"
#include "model/DeviceRegistrationResponseDto.h"
#include "model/DeviceReregistrationResponseDto.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <vector>

using nlohmann::json;

namespace wolkabout
{
/*** CONFIGURATION MANIFEST ***/
void to_json(json& j, const ConfigurationManifest& configurationManifest)
{
    auto dataType = [&]() -> std::string {
        switch (configurationManifest.getDataType())
        {
        case ConfigurationManifest::DataType::BOOLEAN:
            return "BOOLEAN";

        case ConfigurationManifest::DataType::NUMERIC:
            return "NUMERIC";

        case ConfigurationManifest::DataType::STRING:
            return "STRING";

        default:
            assert(false);
            throw std::invalid_argument("Invalid data type");
        }
    }();

    // clang-format off
    j = {
        {"defaultValue", configurationManifest.getDefaultValue()},
        {"dataType", dataType},
        {"description", configurationManifest.getDescription()},
        {"optional", configurationManifest.isOptional()},
        {"nullValue", configurationManifest.getNullValue()},
        {"reference", configurationManifest.getReference()},
        {"unit", configurationManifest.getUnit()},
        {"size", configurationManifest.getSize()},
        {"delimiter", configurationManifest.getDelimiter()},
        {"collapseKey", configurationManifest.getCollapseKey()},
        {"name", configurationManifest.getName()},
        {"maximum", configurationManifest.getMaximum()},
        {"minimum", configurationManifest.getMinimum()}
    };
    // clang-format on
}

void from_json(const json& j, ConfigurationManifest& configurationManifest)
{
    auto dataType = [&]() -> ConfigurationManifest::DataType {
        std::string dataTypeStr = j.at("dataType").get<std::string>();
        if (dataTypeStr == "STRING")
        {
            return ConfigurationManifest::DataType::STRING;
        }
        else if (dataTypeStr == "NUMERIC")
        {
            return ConfigurationManifest::DataType::NUMERIC;
        }
        else if (dataTypeStr == "BOOLEAN")
        {
            return ConfigurationManifest::DataType::BOOLEAN;
        }
        else
        {
            throw std::invalid_argument("Invalid data type");
        }
    }();

    // clang-format off
    configurationManifest =
            ConfigurationManifest(
                j.at("name").get<std::string>(),
                j.at("reference").get<std::string>(),
                j.at("description").get<std::string>(),
                j.at("unit").get<std::string>(),
                dataType,
                j.at("minimum").get<double>(),
                j.at("maximum").get<double>(),
                j.at("collapseKey").get<std::string>(),
                j.at("defaultValue").get<std::string>(),
                j.at("nullValue").get<std::string>(),
                j.at("optional").get<bool>(),
                j.at("size").get<unsigned int>(),
                j.at("delimiter").get<std::string>()
            );
    // clang-format on
}
/*** CONFIGURATION MANIFEST ***/

/*** ALARM MANIFEST ***/
void to_json(json& j, const AlarmManifest& alarmManfiest)
{
    auto alarmSeverity = [&]() -> std::string {
        switch (alarmManfiest.getSeverity())
        {
        case AlarmManifest::AlarmSeverity::ALERT:
            return "ALERT";

        case AlarmManifest::AlarmSeverity::CRITICAL:
            return "CRITICAL";

        case AlarmManifest::AlarmSeverity::ERROR:
            return "ERROR";

        default:
            assert(false);
            throw std::invalid_argument("Invalid alarm severity");
        }
    }();

    // clang-format off
    j = {
        {"reference", alarmManfiest.getReference()},
        {"severity", alarmSeverity},
        {"name", alarmManfiest.getName()},
        {"description", alarmManfiest.getDescription()},
        {"message", alarmManfiest.getMessage()}
    };
    // clang-format on
}

void from_json(const json& j, AlarmManifest& alarmManifest)
{
    auto alarmSeverity = [&]() -> AlarmManifest::AlarmSeverity {
        std::string severity = j.at("severity").get<std::string>();
        if (severity == "ALERT")
        {
            return AlarmManifest::AlarmSeverity::ALERT;
        }
        else if (severity == "ERROR")
        {
            return AlarmManifest::AlarmSeverity::ERROR;
        }
        else if (severity == "CRITICAL")
        {
            return AlarmManifest::AlarmSeverity::CRITICAL;
        }
        else
        {
            throw std::invalid_argument("Invalid alarm severity");
        }
    }();

    // clang-format off
    alarmManifest =
            AlarmManifest(j.at("name").get<std::string>(),
                          alarmSeverity,
                          j.at("reference").get<std::string>(),
                          j.at("message").get<std::string>(),
                          j.at("description").get<std::string>());
    // clang-format on
}
/*** ALARM MANIFEST ***/

/*** ACTUATOR MANIFEST ***/
void to_json(json& j, const ActuatorManifest& actuatorManfiest)
{
    auto dataType = [&]() -> std::string {
        switch (actuatorManfiest.getDataType())
        {
        case ActuatorManifest::DataType::BOOLEAN:
            return "BOOLEAN";

        case ActuatorManifest::DataType::NUMERIC:
            return "NUMERIC";

        case ActuatorManifest::DataType::STRING:
            return "STRING";

        default:
            assert(false);
            throw std::invalid_argument("Invalid data type");
        }
    }();

    auto labels = [&]() -> std::string {
        bool first = true;
        std::stringstream ss;
        for (unsigned long i = 0; i < actuatorManfiest.getLabels().size(); ++i)
        {
            if (!first)
            {
                ss << actuatorManfiest.getDelimiter();
            }

            ss << actuatorManfiest.getLabels().at(i);
            first = false;
        }

        return ss.str();
    }();

    // clang-format off
    j = {
        {"dataType", dataType},
        {"precision", actuatorManfiest.getPrecision()},
        {"description", actuatorManfiest.getDescription()},
        {"readingType", actuatorManfiest.getReadingType()},
        {"labels", labels},
        {"reference", actuatorManfiest.getReference()},
        {"unit", actuatorManfiest.getUnit()},
        {"size", actuatorManfiest.getLabels().size() == 0 ? 1 : actuatorManfiest.getLabels().size()},
        {"delimiter", actuatorManfiest.getDelimiter()},
        {"name", actuatorManfiest.getName()},
        {"minimum", actuatorManfiest.getMinimum()},
        {"maximum", actuatorManfiest.getMaximum()}
    };
    // clang-format on
}

void from_json(const json& j, ActuatorManifest& actuatorManifest)
{
    auto dataType = [&]() -> ActuatorManifest::DataType {
        std::string dataTypeStr = j.at("dataType").get<std::string>();
        if (dataTypeStr == "STRING")
        {
            return ActuatorManifest::DataType::STRING;
        }
        else if (dataTypeStr == "NUMERIC")
        {
            return ActuatorManifest::DataType::NUMERIC;
        }
        else if (dataTypeStr == "BOOLEAN")
        {
            return ActuatorManifest::DataType::BOOLEAN;
        }
        else
        {
            throw std::invalid_argument("Invalid data type");
        }
    }();

    actuatorManifest = ActuatorManifest(
      j.at("name").get<std::string>(), j.at("reference").get<std::string>(), j.at("description").get<std::string>(),
      j.at("unit").get<std::string>(), j.at("readingType").get<std::string>(), dataType,
      j.at("precision").get<unsigned int>(), j.at("minimum").get<long long>(), j.at("maximum").get<long long>());

    std::string delimiter = j.at("delimiter").get<std::string>();
    std::string labelsStr = j.at("labels").get<std::string>();
    std::vector<std::string> labels = StringUtils::tokenize(labelsStr, delimiter);

    if (labels.size() > 0)
    {
        actuatorManifest.setLabels(labels);
        actuatorManifest.setDelimiter(delimiter);
    }
}
/*** ACTUATOR MANIFEST ***/

/*** SENSOR MANIFEST ***/
void to_json(json& j, const SensorManifest& sensorManifest)
{
    auto dataType = [&]() -> std::string {
        switch (sensorManifest.getDataType())
        {
        case SensorManifest::DataType::BOOLEAN:
            return "BOOLEAN";

        case SensorManifest::DataType::NUMERIC:
            return "NUMERIC";

        case SensorManifest::DataType::STRING:
            return "STRING";

        default:
            assert(false);
            throw std::invalid_argument("Invalid data type");
        }
    }();

    auto labels = [&]() -> std::string {
        bool first = true;
        std::stringstream ss;
        for (unsigned long i = 0; i < sensorManifest.getLabels().size(); ++i)
        {
            if (!first)
            {
                ss << sensorManifest.getDelimiter();
            }

            ss << sensorManifest.getLabels().at(i);
            first = false;
        }

        return ss.str();
    }();

    // clang-format off
    j = {
        {"dataType", dataType},
        {"precision", sensorManifest.getPrecision()},
        {"description", sensorManifest.getDescription()},
        {"readingType", sensorManifest.getReadingType()},
        {"labels", labels},
        {"reference", sensorManifest.getReference()},
        {"unit", sensorManifest.getUnit()},
        {"size", sensorManifest.getLabels().size() == 0 ? 1 : sensorManifest.getLabels().size()},
        {"delimiter", sensorManifest.getDelimiter()},
        {"name", sensorManifest.getName()},
        {"minimum", sensorManifest.getMinimum()},
        {"maximum", sensorManifest.getMaximum()}
    };
    // clang-format on
}

void from_json(const json& j, SensorManifest& sensorManifest)
{
    auto dataType = [&]() -> SensorManifest::DataType {
        std::string dataTypeStr = j.at("dataType").get<std::string>();
        if (dataTypeStr == "STRING")
        {
            return SensorManifest::DataType::STRING;
        }
        else if (dataTypeStr == "NUMERIC")
        {
            return SensorManifest::DataType::NUMERIC;
        }
        else if (dataTypeStr == "BOOLEAN")
        {
            return SensorManifest::DataType::BOOLEAN;
        }
        else
        {
            throw std::invalid_argument("Invalid data type");
        }
    }();

    sensorManifest = SensorManifest(
      j.at("name").get<std::string>(), j.at("reference").get<std::string>(), j.at("description").get<std::string>(),
      j.at("unit").get<std::string>(), j.at("readingType").get<std::string>(), dataType,
      j.at("precision").get<unsigned int>(), j.at("minimum").get<long long>(), j.at("maximum").get<long long>());

    std::string delimiter = j.at("delimiter").get<std::string>();
    std::string labelsStr = j.at("labels").get<std::string>();
    std::vector<std::string> labels = StringUtils::tokenize(labelsStr, delimiter);

    if (labels.size() > 0)
    {
        sensorManifest.setLabels(labels);
        sensorManifest.setDelimiter(delimiter);
    }
}
/*** SENSOR MANIFEST ***/

/*** DEVICE MANIFEST ***/
void to_json(json& j, const DeviceManifest& deviceManifest)
{
    // clang-format off
    j = {
        {"name", deviceManifest.getName()},
        {"description", deviceManifest.getDescription()},
        {"protocol", deviceManifest.getProtocol()},
        {"firmwareUpdateProtocol", deviceManifest.getFirmwareUpdateProtocol()},
        {"configs", deviceManifest.getConfigurations()},
        {"alarms", deviceManifest.getAlarms()},
        {"actuators", deviceManifest.getActuators()},
        {"feeds", deviceManifest.getSensors()}
    };
    // clang-format on
}

void from_json(const json& j, DeviceManifest& deviceManifest)
{
    deviceManifest = DeviceManifest(
      j.at("name").get<std::string>(), j.at("description").get<std::string>(), j.at("protocol").get<std::string>(),
      j.at("firmwareUpdateProtocol").get<std::string>(), j.at("configs").get<std::vector<ConfigurationManifest>>(),
      j.at("feeds").get<std::vector<SensorManifest>>(), j.at("alarms").get<std::vector<AlarmManifest>>(),
      j.at("actuators").get<std::vector<ActuatorManifest>>());
}
/*** DEVICE MANIFEST ***/

/*** DEVICE REGISTRATION REQUEST DTO ***/
void to_json(json& j, const DeviceRegistrationRequestDto& dto)
{
    // clang-format off
    j = {
        {"device",
            {{"name", dto.getDeviceName()},
            {"key", dto.getDeviceKey()}}
        },
        {"manifest", dto.getManifest()}
    };
    // clang-format on
}

void from_json(const json& j, DeviceRegistrationRequestDto& dto)
{
    dto =
      DeviceRegistrationRequestDto(j.at("device").at("name").get<std::string>(),
                                   j.at("device").at("key").get<std::string>(), j.at("manifest").get<DeviceManifest>());
}
/*** DEVICE REGISTRATION REQUEST DTO ***/

/*** DEVICE REGISTRATION RESPONSE DTO ***/
void to_json(json& j, const DeviceRegistrationResponseDto& dto)
{
    auto resultStr = [&]() -> std::string {
        switch (dto.getResult())
        {
        case DeviceRegistrationResponseDto::Result::OK:
            return "OK";
            break;

        case DeviceRegistrationResponseDto::Result::ERROR_GATEWAY_NOT_FOUND:
            return "ERROR_GATEWAY_NOT_FOUND";
            break;

        case DeviceRegistrationResponseDto::Result::ERROR_KEY_CONFLICT:
            return "ERROR_KEY_CONFLICT";
            break;

        case DeviceRegistrationResponseDto::Result::ERROR_MANIFEST_CONFLICT:
            return "ERROR_MANIFEST_CONFLICT";
            break;

        case DeviceRegistrationResponseDto::Result::ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED:
            return "ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED";
            break;

        case DeviceRegistrationResponseDto::Result::ERROR_NO_GATEWAY_MANIFEST:
            return "ERROR_NO_GATEWAY_MANIFEST";
            break;

        case DeviceRegistrationResponseDto::Result::ERROR_READING_PAYLOAD:
            return "ERROR_READING_PAYLOAD";
            break;

        default:
            assert(false);
            throw std::invalid_argument("Unhandled result");
        }
    }();

    // clang-format off
    j = {
        {"result", resultStr}
    };
    // clang-format on
}
/*** DEVICE REGISTRATION RESPONSE DTO ***/

/*** DEVICE REREGISTRATION RESPONSE DTO ***/
void to_json(json& j, const DeviceReregistrationResponseDto& dto)
{
    auto resultStr = [&]() -> std::string {
        switch (dto.getResult())
        {
        case DeviceReregistrationResponseDto::Result::OK:
            return "OK";
            break;

        default:
            assert(false);
            throw std::invalid_argument("Unhandled result");
        }
    }();

    // clang-format off
    j = {
        {"result", resultStr}
    };
    // clang-format on
}
/*** DEVICE REREGISTRATION RESPONSE DTO ***/

DeviceRegistrationProtocol::DeviceRegistrationProtocol()
: m_deviceTopics{Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + Channel::DEVICE_PATH_PREFIX +
                 Channel::CHANNEL_DELIMITER + Channel::CHANNEL_WILDCARD}
, m_platformTopics{Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                     Channel::CHANNEL_DELIMITER + Channel::CHANNEL_WILDCARD,
                   Channel::DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                     Channel::CHANNEL_DELIMITER + Channel::CHANNEL_WILDCARD}
{
}

std::vector<std::string> DeviceRegistrationProtocol::getDeviceTopics()
{
    return m_deviceTopics;
}

std::vector<std::string> DeviceRegistrationProtocol::getPlatformTopics()
{
    return m_platformTopics;
}

std::shared_ptr<Message> DeviceRegistrationProtocol::makeMessage(const std::string& gatewayKey,
                                                                 const std::string& deviceKey,
                                                                 const DeviceRegistrationRequestDto& request)
{
    LOG(DEBUG) << METHOD_INFO;

    try
    {
        const json jsonPayload(request);
        const auto channel = [&]() -> std::string {
            if (deviceKey == gatewayKey)
            {
                return Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                       Channel::CHANNEL_DELIMITER + gatewayKey;
            }

            return Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                   Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER + Channel::DEVICE_PATH_PREFIX +
                   Channel::CHANNEL_DELIMITER + deviceKey;
        }();

        return std::make_shared<Message>(jsonPayload.dump(), channel);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to serialize device registration request: " << e.what();
        return nullptr;
    }
}

std::shared_ptr<Message> DeviceRegistrationProtocol::makeMessage(
  const std::string& gatewayKey, const std::string& deviceKey, const wolkabout::DeviceRegistrationResponseDto& response)
{
    LOG(DEBUG) << METHOD_INFO;

    try
    {
        const json jsonPayload(response);
        const auto channel = [&]() -> std::string {
            if (deviceKey == gatewayKey)
            {
                return Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                       Channel::CHANNEL_DELIMITER + gatewayKey;
            }

            return Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                   Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER + Channel::DEVICE_PATH_PREFIX +
                   Channel::CHANNEL_DELIMITER + deviceKey;
        }();

        return std::make_shared<Message>(jsonPayload.dump(), channel);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to serialize device registration response: " << e.what();
        return nullptr;
    }
}

std::shared_ptr<Message> DeviceRegistrationProtocol::makeMessage(const std::string& gatewayKey,
                                                                 const DeviceReregistrationResponseDto& response)
{
    LOG(DEBUG) << METHOD_INFO;

    try
    {
        const json jsonPayload(response);
        const std::string channel = Channel::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                                    Channel::CHANNEL_DELIMITER + gatewayKey;

        return std::make_shared<Message>(jsonPayload.dump(), channel);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to serialize device registration response: " << e.what();
        return nullptr;
    }
}

std::shared_ptr<DeviceRegistrationRequestDto> DeviceRegistrationProtocol::makeRegistrationRequest(
  std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    try
    {
        const json jsonRequest = json::parse(message->getContent());
        auto request = std::make_shared<DeviceRegistrationRequestDto>();
        *request = jsonRequest;
        return request;
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to deserialize device registration request: " << e.what();
        return nullptr;
    }
}

std::shared_ptr<DeviceRegistrationResponseDto> DeviceRegistrationProtocol::makeRegistrationResponse(
  std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    try
    {
        const json j = json::parse(message->getContent());

        const std::string typeStr = j.at("result").get<std::string>();

        const DeviceRegistrationResponseDto::Result result = [&] {
            if (typeStr == REGISTRATION_RESPONSE_OK)
            {
                return DeviceRegistrationResponseDto::Result::OK;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT)
            {
                return DeviceRegistrationResponseDto::Result::ERROR_KEY_CONFLICT;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_MANIFEST_CONFLICT)
            {
                return DeviceRegistrationResponseDto::Result::ERROR_MANIFEST_CONFLICT;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED)
            {
                return DeviceRegistrationResponseDto::Result::ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_READING_PAYLOAD)
            {
                return DeviceRegistrationResponseDto::Result::ERROR_READING_PAYLOAD;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND)
            {
                return DeviceRegistrationResponseDto::Result::ERROR_GATEWAY_NOT_FOUND;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_NO_GATEWAY_MANIFEST)
            {
                return DeviceRegistrationResponseDto::Result::ERROR_NO_GATEWAY_MANIFEST;
            }

            assert(false);
            throw std::logic_error("");
        }();

        return std::make_shared<DeviceRegistrationResponseDto>(result);
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to deserialize device registration response: " << e.what();
        return nullptr;
    }
}

bool DeviceRegistrationProtocol::isMessageToPlatform(const std::string& channel)
{
    LOG(DEBUG) << METHOD_INFO;

    return StringUtils::startsWith(channel, Channel::DEVICE_TO_PLATFORM_DIRECTION);
}

bool DeviceRegistrationProtocol::isMessageFromPlatform(const std::string& channel)
{
    LOG(DEBUG) << METHOD_INFO;

    return StringUtils::startsWith(channel, Channel::PLATFORM_TO_DEVICE_DIRECTION);
}

bool DeviceRegistrationProtocol::isRegistrationRequest(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    return StringUtils::startsWith(message->getChannel(), Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT);
}

bool DeviceRegistrationProtocol::isRegistrationResponse(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    return StringUtils::startsWith(message->getChannel(), Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT);
}

bool DeviceRegistrationProtocol::isReregistrationRequest(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    return StringUtils::startsWith(message->getChannel(), Channel::DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT);
}

bool DeviceRegistrationProtocol::isReregistrationResponse(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    return StringUtils::startsWith(message->getChannel(), Channel::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT);
}

std::string DeviceRegistrationProtocol::extractDeviceKeyFromChannel(const std::string& channel)
{
    LOG(DEBUG) << METHOD_INFO;

    std::string previousToken;
    for (std::string token : StringUtils::tokenize(channel, "/"))
    {
        if (previousToken == "d")
        {
            return token;
        }

        previousToken = token;
    }

    previousToken = "";
    for (std::string token : StringUtils::tokenize(channel, "/"))
    {
        if (previousToken == "g")
        {
            return token;
        }

        previousToken = token;
    }

    return "";
}
}    // namespace wolkabout
