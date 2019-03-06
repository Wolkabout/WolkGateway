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

#include "protocol/json/JsonGatewaySubdeviceRegistrationProtocol.h"
// #include "model/DeviceReregistrationResponse.h"
#include "model/Message.h"
#include "model/SubdeviceRegistrationRequest.h"
#include "model/SubdeviceRegistrationResponse.h"
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
const std::string JsonGatewaySubdeviceRegistrationProtocol::NAME = "RegistrationProtocol";

const std::string JsonGatewaySubdeviceRegistrationProtocol::CHANNEL_DELIMITER = "/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::CHANNEL_MULTI_LEVEL_WILDCARD = "#";
const std::string JsonGatewaySubdeviceRegistrationProtocol::CHANNEL_SINGLE_LEVEL_WILDCARD = "+";
const std::string JsonGatewaySubdeviceRegistrationProtocol::GATEWAY_PATH_PREFIX = "g/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::DEVICE_PATH_PREFIX = "d/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::DEVICE_TO_PLATFORM_DIRECTION = "d2p/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::PLATFORM_TO_DEVICE_DIRECTION = "p2d/";

const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT =
  "d2p/register_subdevice_request/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT =
  "p2d/register_subdevice_response/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT =
  "p2d/reregister_device/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT =
  "d2p/reregister_device/";

const std::string JsonGatewaySubdeviceRegistrationProtocol::DEVICE_DELETION_REQUEST_TOPIC_ROOT = "d2p/delete_device/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::DEVICE_DELETION_RESPONSE_TOPIC_ROOT = "p2d/delete_device/";

const std::string JsonGatewaySubdeviceRegistrationProtocol::REGISTRATION_RESPONSE_OK = "OK";
const std::string JsonGatewaySubdeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT =
  "ERROR_KEY_CONFLICT";
const std::string JsonGatewaySubdeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_MANIFEST_CONFLICT =
  "ERROR_MANIFEST_CONFLICT";
const std::string JsonGatewaySubdeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED =
  "ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED";
const std::string JsonGatewaySubdeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_READING_PAYLOAD =
  "ERROR_READING_PAYLOAD";
const std::string JsonGatewaySubdeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND =
  "ERROR_GATEWAY_NOT_FOUND";
const std::string JsonGatewaySubdeviceRegistrationProtocol::REGISTRATION_RESPONSE_ERROR_NO_GATEWAY_MANIFEST =
  "ERROR_NO_GATEWAY_MANIFEST";

const std::string& JsonGatewaySubdeviceRegistrationProtocol::getName() const
{
    return NAME;
}

std::vector<std::string> JsonGatewaySubdeviceRegistrationProtocol::getInboundPlatformChannels() const
{
    return {SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,

            DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,

            DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewaySubdeviceRegistrationProtocol::getInboundPlatformChannelsForGatewayKey(
  const std::string& gatewayKey) const
{
    return {SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey};
}

std::vector<std::string> JsonGatewaySubdeviceRegistrationProtocol::getInboundPlatformChannelsForKeys(
  const std::string& gatewayKey, const std::string& deviceKey) const
{
    return {SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + deviceKey,
            SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + deviceKey,
            DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + deviceKey,
            DEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey};
}

std::vector<std::string> JsonGatewaySubdeviceRegistrationProtocol::getInboundDeviceChannels() const
{
    return {SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewaySubdeviceRegistrationProtocol::getInboundDeviceChannelsForDeviceKey(
  const std::string& deviceKey) const
{
    return {SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey};
}

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeMessage(
  const std::string& gatewayKey, const std::string& deviceKey, const SubdeviceRegistrationRequest& request) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonPayload(request);
        const auto channel = [&]() -> std::string {
            if (deviceKey == gatewayKey)
            {
                return SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
            }

            return SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
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

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeMessage(
  const std::string& deviceKey, const wolkabout::SubdeviceRegistrationResponse& response) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonPayload(response);
        const auto channel = SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey;

        return std::unique_ptr<Message>(new Message(jsonPayload.dump(), channel));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to serialize device registration response: " << e.what();
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeMessage(
  const std::string& gatewayKey, const std::string& deviceKey,
  const wolkabout::SubdeviceRegistrationResponse& response) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonPayload(response);
        const auto channel = SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey +
                             CHANNEL_DELIMITER + DEVICE_PATH_PREFIX + deviceKey;

        return std::unique_ptr<Message>(new Message(jsonPayload.dump(), channel));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to serialize device registration response: " << e.what();
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeMessage(
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

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeDeviceReregistrationRequestForDevice() const
{
    const std::string channel = DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX;
    return std::unique_ptr<Message>(new Message("", channel));
}

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeDeviceReregistrationRequestForGateway(
  const std::string& gatewayKey) const
{
    const std::string channel = DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
    return std::unique_ptr<Message>(new Message("", channel));
}

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeDeviceDeletionRequestMessage(
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

std::unique_ptr<SubdeviceRegistrationRequest> JsonGatewaySubdeviceRegistrationProtocol::makeRegistrationRequest(
  const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonRequest = json::parse(message.getContent());
        SubdeviceRegistrationRequest request;
        request = jsonRequest;

        return std::unique_ptr<SubdeviceRegistrationRequest>(new SubdeviceRegistrationRequest(request));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to deserialize device registration request: " << e.what();
        return nullptr;
    }
}

std::unique_ptr<SubdeviceRegistrationResponse> JsonGatewaySubdeviceRegistrationProtocol::makeRegistrationResponse(
  const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json j = json::parse(message.getContent());

        const std::string typeStr = j.at("result").get<std::string>();

        const SubdeviceRegistrationResponse::Result result = [&] {
            if (typeStr == REGISTRATION_RESPONSE_OK)
            {
                return SubdeviceRegistrationResponse::Result::OK;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT)
            {
                return SubdeviceRegistrationResponse::Result::ERROR_KEY_CONFLICT;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_MANIFEST_CONFLICT)
            {
                return SubdeviceRegistrationResponse::Result::ERROR_MANIFEST_CONFLICT;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED)
            {
                return SubdeviceRegistrationResponse::Result::ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_READING_PAYLOAD)
            {
                return SubdeviceRegistrationResponse::Result::ERROR_READING_PAYLOAD;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND)
            {
                return SubdeviceRegistrationResponse::Result::ERROR_GATEWAY_NOT_FOUND;
            }
            else if (typeStr == REGISTRATION_RESPONSE_ERROR_NO_GATEWAY_MANIFEST)
            {
                return SubdeviceRegistrationResponse::Result::ERROR_NO_GATEWAY_MANIFEST;
            }

            assert(false);
            throw std::logic_error("");
        }();

        return std::unique_ptr<SubdeviceRegistrationResponse>(new SubdeviceRegistrationResponse(result));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Device registration protocol: Unable to deserialize device registration response: " << e.what();
        return nullptr;
    }
}

bool JsonGatewaySubdeviceRegistrationProtocol::isMessageToPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_TO_PLATFORM_DIRECTION);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isMessageFromPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), PLATFORM_TO_DEVICE_DIRECTION);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isRegistrationRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isRegistrationResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isReregistrationRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isReregistrationResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isDeviceDeletionRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_DELETION_REQUEST_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isDeviceDeletionResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_DELETION_RESPONSE_TOPIC_ROOT);
}

std::string JsonGatewaySubdeviceRegistrationProtocol::getResponseChannel(const Message& message,
                                                                         const std::string& gatewayKey,
                                                                         const std::string& deviceKey) const
{
    LOG(TRACE) << METHOD_INFO;

    if (isRegistrationRequest(message))
    {
        if (gatewayKey == deviceKey)
        {
            return SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
        }
        else
        {
            return SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
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

std::string JsonGatewaySubdeviceRegistrationProtocol::extractDeviceKeyFromChannel(const std::string& channel) const
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
