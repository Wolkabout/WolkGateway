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

#include "RegistrationProtocol.h"
#include "Poco/Bugcheck.h"
#include "connectivity/Channels.h"
#include "model/DeviceRegistrationRequestDto.h"
#include "model/DeviceRegistrationResponseDto.h"
#include "model/DeviceReregistrationResponseDto.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"

#include <algorithm>
#include <stdexcept>

using nlohmann::json;

namespace wolkabout
{
const std::string RegistrationProtocol::REGISTRATION_RESPONSE_OK = "OK";
const std::string RegistrationProtocol::REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT = "ERROR_KEY_CONFLICT";
const std::string RegistrationProtocol::REGISTRATION_RESPONSE_ERROR_MANIFEST_CONFLICT = "ERROR_MANIFEST_CONFLICT";
const std::string RegistrationProtocol::REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED =
  "ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED";
const std::string RegistrationProtocol::REGISTRATION_RESPONSE_ERROR_READING_PAYLOAD = "ERROR_READING_PAYLOAD";
const std::string RegistrationProtocol::REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND = "ERROR_GATEWAY_NOT_FOUND";
const std::string RegistrationProtocol::REGISTRATION_RESPONSE_ERROR_NO_GATEWAY_MANIFEST = "ERROR_NO_GATEWAY_MANIFEST";

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
            poco_assert_dbg(false);
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
            poco_assert_dbg(false);
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
            poco_assert_dbg(false);
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
        std::string dataType = j.at("dataType").get<std::string>();
        if (dataType == "STRING")
        {
            return ActuatorManifest::DataType::STRING;
        }
        else if (dataType == "NUMERIC")
        {
            return ActuatorManifest::DataType::NUMERIC;
        }
        else if (dataType == "BOOLEAN")
        {
            return ActuatorManifest::DataType::BOOLEAN;
        }
        else
        {
            throw std::invalid_argument("Invalid data type");
        }
    }();

    actuatorManifest = ActuatorManifest(
      j.at("dataType").get<std::string>(), j.at("reference").get<std::string>(), j.at("description").get<std::string>(),
      j.at("unit").get<std::string>(), j.at("readingType").get<std::string>(), dataType,
      j.at("precision").get<unsigned int>(), j.at("minimum").get<long long>(), j.at("maximum").get<long long>());

    // TODO: String utils get labels and delimiter
    std::initializer_list<std::string> labels = {};
    std::string delimiter = "_";

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
            poco_assert_dbg(false);
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
        std::string dataType = j.at("dataType").get<std::string>();
        if (dataType == "STRING")
        {
            return SensorManifest::DataType::STRING;
        }
        else if (dataType == "NUMERIC")
        {
            return SensorManifest::DataType::NUMERIC;
        }
        else if (dataType == "BOOLEAN")
        {
            return SensorManifest::DataType::BOOLEAN;
        }
        else
        {
            throw std::invalid_argument("Invalid data type");
        }
    }();

    sensorManifest = SensorManifest(
      j.at("dataType").get<std::string>(), j.at("reference").get<std::string>(), j.at("description").get<std::string>(),
      j.at("unit").get<std::string>(), j.at("readingType").get<std::string>(), dataType,
      j.at("precision").get<unsigned int>(), j.at("minimum").get<long long>(), j.at("maximum").get<long long>());

    // TODO: String utils get labels and delimiter
    std::initializer_list<std::string> labels = {};
    std::string delimiter = "_";

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
    dto = DeviceRegistrationRequestDto(
      j.at("name").get<std::string>(), j.at("key").get<std::string>(), j.at("manifest").get<DeviceManifest>());
}
/*** DEVICE REGISTRATION REQUEST DTO ***/

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
            poco_assert_dbg(false);
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

RegistrationProtocol::RegistrationProtocol()
: m_devicTopics{Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + Channel::CHANNEL_WILDCARD,
                Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + Channel::CHANNEL_WILDCARD}
, m_platformTopics{Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + Channel::CHANNEL_WILDCARD,
                   Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + Channel::CHANNEL_WILDCARD}
, m_deviceMessageTypes{Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT,
                       Channel::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT}
, m_platformMessageTypes{Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT,
                         Channel::DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT}
{
}

std::vector<std::string> RegistrationProtocol::getDeviceTopics()
{
    return m_devicTopics;
}

std::vector<std::string> RegistrationProtocol::getPlatformTopics()
{
    return m_platformTopics;
}

std::shared_ptr<Message> RegistrationProtocol::make(const std::string& gatewayKey, const std::string& deviceKey,
                                                    const DeviceRegistrationRequestDto& request)
{
    LOG(DEBUG) << METHOD_INFO;

    try
    {
        const json jsonPayload(request);
        const std::string channel = Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                                    Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER +
                                    Channel::DEVICE_PATH_PREFIX + Channel::CHANNEL_DELIMITER + deviceKey;

        return std::make_shared<Message>(jsonPayload.dump(), channel);
    }
    catch (...)
    {
        return nullptr;
    }
}

std::shared_ptr<Message> RegistrationProtocol::make(const std::string& gatewayKey, const std::string& deviceKey,
                                                    const DeviceReregistrationResponseDto& response)
{
    LOG(DEBUG) << METHOD_INFO;

    try
    {
        const json jsonPayload(response);
        const std::string channel = Channel::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT + Channel::GATEWAY_PATH_PREFIX +
                                    Channel::CHANNEL_DELIMITER + gatewayKey + Channel::CHANNEL_DELIMITER +
                                    Channel::DEVICE_PATH_PREFIX + Channel::CHANNEL_DELIMITER + deviceKey;

        return std::make_shared<Message>(jsonPayload.dump(), channel);
    }
    catch (...)
    {
    }
}

std::shared_ptr<DeviceRegistrationRequestDto> RegistrationProtocol::makeRegistrationRequest(
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
    catch (...)
    {
        LOG(DEBUG) << "Registration protocol: Unable to deserialize device registration request: "
                   << message->getContent();
        return nullptr;
    }
}

std::shared_ptr<DeviceRegistrationResponseDto> RegistrationProtocol::makeRegistrationResponse(
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

            poco_assert_dbg(false);
            throw std::logic_error("");
        }();

        return std::make_shared<DeviceRegistrationResponseDto>(result);
    }
    catch (...)
    {
        LOG(DEBUG) << "Registration protocol: Unable to parse DeviceRegistrationResponseDto: " << message->getContent();
        return nullptr;
    }
}

bool RegistrationProtocol::isGatewayToPlatformMessage(const std::string& topic, const std::string& gatewayKey)
{
    LOG(DEBUG) << METHOD_INFO;

    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 4)
    {
        LOG(TRACE) << "Registration protocol: Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::DEVICE_TO_PLATFORM_DIRECTION)
    {
        LOG(TRACE) << "Registration protocol: Dirrection mismatch in path: " << topic;
        return false;
    }

    if (std::find(m_deviceMessageTypes.begin(), m_deviceMessageTypes.end(), tokens[TYPE_POS]) ==
        m_deviceMessageTypes.end())
    {
        LOG(TRACE) << "Registration protocol: Device message type not supported: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(TRACE) << "Registration protocol: Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_KEY_POS] != gatewayKey)
    {
        LOG(TRACE) << "Registration protocol: Gateway key mismatch in path: " << topic;
        return false;
    }

    return true;
}

bool RegistrationProtocol::isPlatformToGatewayMessage(const std::string& topic, const std::string& gatewayKey)
{
    LOG(DEBUG) << METHOD_INFO;

    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 4)
    {
        LOG(TRACE) << "Registration protocol: Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::PLATFORM_TO_DEVICE_DIRECTION)
    {
        LOG(TRACE) << "Registration protocol: Dirrection mismatch in path: " << topic;
        return false;
    }

    if (std::find(m_platformMessageTypes.begin(), m_platformMessageTypes.end(), tokens[TYPE_POS]) ==
        m_platformMessageTypes.end())
    {
        LOG(TRACE) << "Registration protocol: Device message type not supported: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(TRACE) << "Registration protocol: Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_KEY_POS] != gatewayKey)
    {
        LOG(TRACE) << "Registration protocol: Gateway key does not match in path: " << topic;
        return false;
    }

    return true;
}

bool RegistrationProtocol::isDeviceToPlatformMessage(const std::string& topic)
{
    LOG(DEBUG) << METHOD_INFO

      auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 4)
    {
        LOG(TRACE) << "Registration protocol: Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::DEVICE_TO_PLATFORM_DIRECTION)
    {
        LOG(TRACE) << "Registration protocol: Dirrection mismatch in path: " << topic;
        return false;
    }

    if (std::find(m_deviceMessageTypes.begin(), m_deviceMessageTypes.end(), tokens[TYPE_POS]) ==
        m_deviceMessageTypes.end())
    {
        LOG(DEBUG) << "Registration protocol: Device message type not supported: " << topic;
        return false;
    }

    if (tokens[DEVICE_TYPE_POS] != Channel::DEVICE_PATH_PREFIX)
    {
        LOG(DEBUG) << "Registration protocol: Device perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool RegistrationProtocol::isMessageToPlatform(const std::string& topic, const std::string& gatewayKey)
{
    LOG(DEBUG) << METHOD_INFO;

    return isDeviceToPlatformMessage(topic) || isGatewayToPlatformMessage(topic, gatewayKey);
}

bool RegistrationProtocol::isPlatformToDeviceMessage(const std::string& topic, const std::string& gatewayKey)
{
    LOG(DEBUG) << METHOD_INFO;

    auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 6)
    {
        LOG(TRACE) << "Registration protocol: Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::PLATFORM_TO_DEVICE_DIRECTION)
    {
        LOG(TRACE) << "Registration protocol: Dirrection mismatch in path: " << topic;
        return false;
    }

    if (std::find(m_platformMessageTypes.begin(), m_platformMessageTypes.end(), tokens[TYPE_POS]) ==
        m_platformMessageTypes.end())
    {
        LOG(TRACE) << "Registration protocol: Device message type not supported: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(TRACE) << "Registration protocol: Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_KEY_POS] != gatewayKey)
    {
        LOG(TRACE) << "Registration protocol: Gateway key mismatch in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_DEVICE_TYPE_POS] != Channel::DEVICE_PATH_PREFIX)
    {
        LOG(DEBUG) << "Registration protocol: Device perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool RegistrationProtocol::isMessageFromPlatform(const std::string& topic, const std::string& gatewayKey)
{
    LOG(DEBUG) << METHOD_INFO;

    return isPlatformToDeviceMessage(topic, gatewayKey) || isPlatformToGatewayMessage(topic, gatewayKey);
}

bool RegistrationProtocol::isRegistrationRequest(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    return StringUtils::mqttTopicMatch(Channel::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT, message->getChannel());
}

bool RegistrationProtocol::isRegistrationResponse(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    return StringUtils::mqttTopicMatch(Channel::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT, message->getChannel());
}

bool RegistrationProtocol::isReregistrationRequest(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    return StringUtils::mqttTopicMatch(Channel::DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT, message->getChannel());
}

bool RegistrationProtocol::isReregistrationResponse(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    return StringUtils::mqttTopicMatch(Channel::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT, message->getChannel());
}

std::string RegistrationProtocol::getDeviceKeyFromChannel(const std::string& channel)
{
    LOG(DEBUG) << METHOD_INFO;

    std::string previousToken;
    // Device related message
    for (std::string token : StringUtils::tokenize(channel, "/"))
    {
        if (previousToken == "d")
        {
            return token;
        }

        previousToken = token;
    }

    // Gateway related message
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
