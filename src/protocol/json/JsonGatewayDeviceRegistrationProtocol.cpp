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

#include "protocol/json/JsonGatewayDeviceRegistrationProtocol.h"
#include "model/DeviceRegistrationRequest.h"
#include "model/DeviceRegistrationResponse.h"
#include "model/DeviceReregistrationResponse.h"
#include "model/Message.h"
#include "protocol/json/JsonDto.h"
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
const std::string JsonGatewayDeviceRegistrationProtocol::NAME = "RegistrationProtocol";

const std::string JsonGatewayDeviceRegistrationProtocol::CHANNEL_DELIMITER = "/";
const std::string JsonGatewayDeviceRegistrationProtocol::CHANNEL_MULTI_LEVEL_WILDCARD = "#";
const std::string JsonGatewayDeviceRegistrationProtocol::CHANNEL_SINGLE_LEVEL_WILDCARD = "+";
const std::string JsonGatewayDeviceRegistrationProtocol::GATEWAY_PATH_PREFIX = "g/";
const std::string JsonGatewayDeviceRegistrationProtocol::DEVICE_PATH_PREFIX = "d/";
const std::string JsonGatewayDeviceRegistrationProtocol::DEVICE_TO_PLATFORM_DIRECTION = "d2p/";
const std::string JsonGatewayDeviceRegistrationProtocol::PLATFORM_TO_DEVICE_DIRECTION = "p2d/";

const std::string JsonGatewayDeviceRegistrationProtocol::DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT =
  "d2p/register_device/";
const std::string JsonGatewayDeviceRegistrationProtocol::DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT =
  "p2d/register_device/";
const std::string JsonGatewayDeviceRegistrationProtocol::DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT =
  "p2d/reregister_device/";
const std::string JsonGatewayDeviceRegistrationProtocol::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT =
  "d2p/reregister_device/";

const std::string JsonGatewayDeviceRegistrationProtocol::DEVICE_DELETION_REQUEST_TOPIC_ROOT = "d2p/delete_device/";
const std::string JsonGatewayDeviceRegistrationProtocol::DEVICE_DELETION_RESPONSE_TOPIC_ROOT = "p2d/delete_device/";

const std::string JsonGatewayDeviceRegistrationProtocol::REGISTRATION_RESPONSE_OK = "OK";
const std::string JsonGatewayDeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT =
  "ERROR_KEY_CONFLICT";
const std::string JsonGatewayDeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_MANIFEST_CONFLICT =
  "ERROR_MANIFEST_CONFLICT";
const std::string JsonGatewayDeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED =
  "ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED";
const std::string JsonGatewayDeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_READING_PAYLOAD =
  "ERROR_READING_PAYLOAD";
const std::string JsonGatewayDeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND =
  "ERROR_GATEWAY_NOT_FOUND";
const std::string JsonGatewayDeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_NO_GATEWAY_MANIFEST =
  "ERROR_NO_GATEWAY_MANIFEST";

const std::string& JsonGatewayDeviceRegistrationProtocol::getName() const
{
    return NAME;
}

std::vector<std::string> JsonGatewayDeviceRegistrationProtocol::getInboundPlatformChannels() const
{
    return {DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,

            DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,

            DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewayDeviceRegistrationProtocol::getInboundPlatformChannelsForGatewayKey(
  const std::string& gatewayKey) const
{
    return {DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey};
}

std::vector<std::string> JsonGatewayDeviceRegistrationProtocol::getInboundPlatformChannelsForKeys(
  const std::string& gatewayKey, const std::string& deviceKey) const
{
    return {DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + deviceKey,
            DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + deviceKey,
            DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + deviceKey,
            DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey};
}

std::vector<std::string> JsonGatewayDeviceRegistrationProtocol::getInboundDeviceChannels() const
{
    return {DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewayDeviceRegistrationProtocol::getInboundDeviceChannelsForDeviceKey(
  const std::string& deviceKey) const
{
    return {DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey};
}

std::unique_ptr<Message> JsonGatewayDeviceRegistrationProtocol::makeMessage(
  const std::string& gatewayKey, const std::string& deviceKey, const DeviceRegistrationRequest& request) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonPayload(request);
        const auto channel = [&]() -> std::string {
            if (deviceKey == gatewayKey)
            {
                return DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
            }

            return DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                   DEVICE_PATH_PREFIX + deviceKey;
        }();

        auto r = jsonPayload.dump();

        return std::unique_ptr<Message>(new Message(r, channel));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to serialize device registration request: " << e.what();
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewayDeviceRegistrationProtocol::makeMessage(
  const std::string& deviceKey, const wolkabout::DeviceRegistrationResponse& response) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonPayload(response);
        const auto channel = DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey;

        return std::unique_ptr<Message>(new Message(jsonPayload.dump(), channel));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to serialize device registration response: " << e.what();
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewayDeviceRegistrationProtocol::makeMessage(
  const std::string& gatewayKey, const std::string& deviceKey,
  const wolkabout::DeviceRegistrationResponse& response) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonPayload(response);
        const auto channel = DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey +
                             CHANNEL_DELIMITER + DEVICE_PATH_PREFIX + deviceKey;

        return std::unique_ptr<Message>(new Message(jsonPayload.dump(), channel));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to serialize device registration response: " << e.what();
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewayDeviceRegistrationProtocol::makeMessage(
  const std::string& gatewayKey, const DeviceReregistrationResponse& response) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonPayload(response);
        const std::string channel = DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;

        return std::unique_ptr<Message>(new Message(jsonPayload.dump(), channel));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to serialize device registration response: " << e.what();
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewayDeviceRegistrationProtocol::makeDeviceReregistrationRequestForDevice() const
{
    const std::string channel = DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX;
    return std::unique_ptr<Message>(new Message("", channel));
}

std::unique_ptr<Message> JsonGatewayDeviceRegistrationProtocol::makeDeviceReregistrationRequestForGateway(
  const std::string& gatewayKey) const
{
    const std::string channel = DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
    return std::unique_ptr<Message>(new Message("", channel));
}

std::unique_ptr<Message> JsonGatewayDeviceRegistrationProtocol::makeDeviceDeletionRequestMessage(
  const std::string& gatewayKey, const std::string& deviceKey) const
{
    std::stringstream channel;
    channel << DEVICE_DELETION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
    if (deviceKey != gatewayKey)
    {
        channel << CHANNEL_DELIMITER + DEVICE_PATH_PREFIX + deviceKey;
    }

    return std::unique_ptr<Message>(new Message("", channel.str()));
}

std::unique_ptr<DeviceRegistrationRequest> JsonGatewayDeviceRegistrationProtocol::makeRegistrationRequest(
  const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonRequest = json::parse(message.getContent());
        DeviceRegistrationRequest request;
        request = jsonRequest;

        return std::unique_ptr<DeviceRegistrationRequest>(new DeviceRegistrationRequest(request));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to deserialize device registration request: " << e.what();
        return nullptr;
    }
}

std::unique_ptr<DeviceRegistrationResponse> JsonGatewayDeviceRegistrationProtocol::makeRegistrationResponse(
  const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json j = json::parse(message.getContent());

        const std::string typeStr = j.at("result").get<std::string>();

        const DeviceRegistrationResponse::Result result = [&] {
            if (typeStr == REGISTRATION_RESPONSE_OK)
            {
                return DeviceRegistrationResponse::Result::OK;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT)
            {
                return DeviceRegistrationResponse::Result::ERROR_KEY_CONFLICT;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_MANIFEST_CONFLICT)
            {
                return DeviceRegistrationResponse::Result::ERROR_MANIFEST_CONFLICT;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED)
            {
                return DeviceRegistrationResponse::Result::ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_READING_PAYLOAD)
            {
                return DeviceRegistrationResponse::Result::ERROR_READING_PAYLOAD;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND)
            {
                return DeviceRegistrationResponse::Result::ERROR_GATEWAY_NOT_FOUND;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_NO_GATEWAY_MANIFEST)
            {
                return DeviceRegistrationResponse::Result::ERROR_NO_GATEWAY_MANIFEST;
            }

            assert(false);
            throw std::logic_error("");
        }();

        return std::unique_ptr<DeviceRegistrationResponse>(new DeviceRegistrationResponse(result));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to deserialize device registration response: " << e.what();
        return nullptr;
    }
}

bool JsonGatewayDeviceRegistrationProtocol::isMessageToPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_TO_PLATFORM_DIRECTION);
}

bool JsonGatewayDeviceRegistrationProtocol::isMessageFromPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), PLATFORM_TO_DEVICE_DIRECTION);
}

bool JsonGatewayDeviceRegistrationProtocol::isRegistrationRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT);
}

bool JsonGatewayDeviceRegistrationProtocol::isRegistrationResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT);
}

bool JsonGatewayDeviceRegistrationProtocol::isReregistrationRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT);
}

bool JsonGatewayDeviceRegistrationProtocol::isReregistrationResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT);
}

bool JsonGatewayDeviceRegistrationProtocol::isDeviceDeletionRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_DELETION_REQUEST_TOPIC_ROOT);
}

bool JsonGatewayDeviceRegistrationProtocol::isDeviceDeletionResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_DELETION_RESPONSE_TOPIC_ROOT);
}

std::string JsonGatewayDeviceRegistrationProtocol::getResponseChannel(const Message& message,
                                                                      const std::string& gatewayKey,
                                                                      const std::string& deviceKey) const
{
    LOG(TRACE) << METHOD_INFO;

    if (isRegistrationRequest(message))
    {
        if (gatewayKey == deviceKey)
        {
            return DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
        }
        else
        {
            return DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                   DEVICE_PATH_PREFIX + deviceKey;
        }
    }
    else if (isDeviceDeletionRequest(message))
    {
        if (gatewayKey == deviceKey)
        {
            return DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
        }
        else
        {
            return DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                   DEVICE_PATH_PREFIX + deviceKey;
        }
    }

    return "";
}

std::string JsonGatewayDeviceRegistrationProtocol::extractDeviceKeyFromChannel(const std::string& channel) const
{
    LOG(TRACE) << METHOD_INFO;

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
