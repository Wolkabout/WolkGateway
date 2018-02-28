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
#include "connectivity/Channels.h"
#include "model/DeviceRegistrationRequestDto.h"
#include "model/DeviceRegistrationResponseDto.h"
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
const std::string RegistrationProtocol::REGISTRATION_RESPONSE_KEY_CONFLICT = "KEY_CONFLICT";
const std::string RegistrationProtocol::REGISTRATION_RESPONSE_KEY_AND_MANIFEST_CONFLICT = "KEY_AND_MANIFEST_CONFLICT";
const std::string RegistrationProtocol::REGISTRATION_RESPONSE_MAX_NUMBER_OF_DEVICES_EXCEEDED =
  "MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED";

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

std::shared_ptr<Message> RegistrationProtocol::make(const std::string& gatewayKey,
                                                    std::shared_ptr<DeviceRegistrationRequestDto> request)
{
    return nullptr;
}

std::shared_ptr<Message> RegistrationProtocol::make(const std::string& gatewayKey,
                                                    std::shared_ptr<DeviceRegistrationResponseDto> response)
{
    return nullptr;
}

std::shared_ptr<DeviceRegistrationRequestDto> RegistrationProtocol::makeRegistrationRequest(
  std::shared_ptr<Message> message)
{
    return nullptr;
}

std::shared_ptr<DeviceRegistrationResponseDto> RegistrationProtocol::makeRegistrationResponse(
  std::shared_ptr<Message> message)
{
    try
    {
        const json j = json::parse(message->getContent());

        const std::string typeStr = j.at("result").get<std::string>();

        const DeviceRegistrationResponseDto::Result result = [&] {
            if (typeStr == REGISTRATION_RESPONSE_OK)
            {
                return DeviceRegistrationResponseDto::Result::OK;
            }
            else if (typeStr == REGISTRATION_RESPONSE_KEY_CONFLICT)
            {
                return DeviceRegistrationResponseDto::Result::KEY_CONFLICT;
            }
            else if (typeStr == REGISTRATION_RESPONSE_KEY_AND_MANIFEST_CONFLICT)
            {
                return DeviceRegistrationResponseDto::Result::KEY_AND_MANIFEST_CONFLICT;
            }
            else if (typeStr == REGISTRATION_RESPONSE_MAX_NUMBER_OF_DEVICES_EXCEEDED)
            {
                return DeviceRegistrationResponseDto::Result::MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED;
            }

            throw std::logic_error("");
        }();

        return std::make_shared<DeviceRegistrationResponseDto>(result);
    }
    catch (...)
    {
        LOG(DEBUG) << "Unable to parse DeviceRegistrationResponseDto: " << message->getContent();
        return nullptr;
    }
}

bool RegistrationProtocol::isGatewayToPlatformMessage(const std::string& topic, const std::string& gatewayKey)
{
    LOG(DEBUG) << METHOD_INFO

      auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 4)
    {
        LOG(TRACE) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::DEVICE_TO_PLATFORM_DIRECTION)
    {
        LOG(TRACE) << "Dirrection mismatch in path: " << topic;
        return false;
    }

    if (std::find(m_deviceMessageTypes.begin(), m_deviceMessageTypes.end(), tokens[TYPE_POS]) ==
        m_deviceMessageTypes.end())
    {
        LOG(TRACE) << "Device message type not supported: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(TRACE) << "Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_KEY_POS] != gatewayKey)
    {
        LOG(TRACE) << "Gateway key mismatch in path: " << topic;
        return false;
    }

    return true;
}

bool RegistrationProtocol::isPlatformToGatewayMessage(const std::string& topic, const std::string& gatewayKey)
{
    LOG(DEBUG) << METHOD_INFO

      auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 4)
    {
        LOG(TRACE) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::PLATFORM_TO_DEVICE_DIRECTION)
    {
        LOG(TRACE) << "Dirrection mismatch in path: " << topic;
        return false;
    }

    if (std::find(m_platformMessageTypes.begin(), m_platformMessageTypes.end(), tokens[TYPE_POS]) ==
        m_platformMessageTypes.end())
    {
        LOG(TRACE) << "Device message type not supported: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(TRACE) << "Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_KEY_POS] != gatewayKey)
    {
        LOG(TRACE) << "Gateway key does not match in path: " << topic;
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
        LOG(TRACE) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::DEVICE_TO_PLATFORM_DIRECTION)
    {
        LOG(TRACE) << "Dirrection mismatch in path: " << topic;
        return false;
    }

    if (std::find(m_deviceMessageTypes.begin(), m_deviceMessageTypes.end(), tokens[TYPE_POS]) ==
        m_deviceMessageTypes.end())
    {
        LOG(DEBUG) << "Device message type not supported: " << topic;
        return false;
    }

    if (tokens[DEVICE_TYPE_POS] != Channel::DEVICE_PATH_PREFIX)
    {
        LOG(DEBUG) << "Device perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool RegistrationProtocol::isPlatformToDeviceMessage(const std::string& topic, const std::string& gatewayKey)
{
    LOG(DEBUG) << METHOD_INFO

      auto tokens = StringUtils::tokenize(topic, Channel::CHANNEL_DELIMITER);

    if (tokens.size() != 6)
    {
        LOG(TRACE) << "Token count mismatch in path: " << topic;
        return false;
    }

    if (tokens[DIRRECTION_POS] != Channel::PLATFORM_TO_DEVICE_DIRECTION)
    {
        LOG(TRACE) << "Dirrection mismatch in path: " << topic;
        return false;
    }

    if (std::find(m_platformMessageTypes.begin(), m_platformMessageTypes.end(), tokens[TYPE_POS]) ==
        m_platformMessageTypes.end())
    {
        LOG(TRACE) << "Device message type not supported: " << topic;
        return false;
    }

    if (tokens[GATEWAY_TYPE_POS] != Channel::GATEWAY_PATH_PREFIX)
    {
        LOG(TRACE) << "Gateway perfix missing in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_KEY_POS] != gatewayKey)
    {
        LOG(TRACE) << "Gateway key mismatch in path: " << topic;
        return false;
    }

    if (tokens[GATEWAY_DEVICE_TYPE_POS] != Channel::DEVICE_PATH_PREFIX)
    {
        LOG(DEBUG) << "Device perfix missing in path: " << topic;
        return false;
    }

    return true;
}

bool RegistrationProtocol::isRegistrationRequest(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO

      return StringUtils::mqttTopicMatch(Channel::DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT, message->getTopic());
}

bool RegistrationProtocol::isRegistrationResponse(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO

      return StringUtils::mqttTopicMatch(Channel::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT, message->getTopic());
}
}
