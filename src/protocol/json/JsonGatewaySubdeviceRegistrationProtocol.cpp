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
#include "model/GatewayUpdateRequest.h"
#include "model/GatewayUpdateResponse.h"
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

const std::string JsonGatewaySubdeviceRegistrationProtocol::GATEWAY_UPDATE_REQUEST_TOPIC_ROOT =
  "d2p/update_gateway_request/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::GATEWAY_UPDATE_RESPONSE_TOPIC_ROOT =
  "p2d/update_gateway_response/";

const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_DELETION_REQUEST_TOPIC_ROOT =
  "d2p/delete_subdevice_request/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_DELETION_RESPONSE_TOPIC_ROOT =
  "p2d/delete_subdevice_response/";

const std::string JsonGatewaySubdeviceRegistrationProtocol::GATEWAY_UPDATE_RESPONSE_OK = "OK";
const std::string JsonGatewaySubdeviceRegistrationProtocol::GATEWAY_UPDATE_RESPONSE_ERROR_GATEWAY_NOT_FOUND =
  "ERROR_GATEWAY_NOT_FOUND";
const std::string JsonGatewaySubdeviceRegistrationProtocol::GATEWAY_UPDATE_RESPONSE_ERROR_NOT_A_GATEWAY =
  "ERROR_NOT_A_GATEWAY";
const std::string JsonGatewaySubdeviceRegistrationProtocol::GATEWAY_UPDATE_RESPONSE_ERROR_VALIDATION_ERROR =
  "ERROR_VALIDATION_ERROR";
const std::string JsonGatewaySubdeviceRegistrationProtocol::GATEWAY_UPDATE_RESPONSE_ERROR_INVALID_DTO =
  "ERROR_INVALID_DTO";
const std::string JsonGatewaySubdeviceRegistrationProtocol::GATEWAY_UPDATE_RESPONSE_ERROR_KEY_MISSING =
  "ERROR_KEY_MISSING";
const std::string JsonGatewaySubdeviceRegistrationProtocol::GATEWAY_UPDATE_RESPONSE_ERROR_UNKNOWN = "ERROR_UNKNOWN";

const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_OK = "OK";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND =
  "ERROR_GATEWAY_NOT_FOUND";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_ERROR_NOT_A_GATEWAY =
  "ERROR_NOT_A_GATEWAY";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT =
  "ERROR_KEY_CONFLICT";
const std::string
  JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED =
    "ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_ERROR_VALIDATION_ERROR =
  "ERROR_VALIDATION_ERROR";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_ERROR_INVALID_DTO =
  "ERROR_INVALID_DTO";
const std::string
  JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_ERROR_SUBDEVICE_MANAGEMENT_FORBIDDEN =
    "ERROR_SUBDEVICE_MANAGEMENT_FORBIDDEN";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_ERROR_KEY_MISSING =
  "ERROR_KEY_MISSING";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_ERROR_UNKNOWN =
  "ERROR_UNKNOWN";

const std::string& JsonGatewaySubdeviceRegistrationProtocol::getName() const
{
    return NAME;
}

std::vector<std::string> JsonGatewaySubdeviceRegistrationProtocol::getInboundPlatformChannels() const
{
    return {SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,

            GATEWAY_UPDATE_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            SUBDEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewaySubdeviceRegistrationProtocol::getInboundPlatformChannelsForGatewayKey(
  const std::string& gatewayKey) const
{
    return {SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            GATEWAY_UPDATE_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            SUBDEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewaySubdeviceRegistrationProtocol::getInboundPlatformChannelsForKeys(
  const std::string& gatewayKey, const std::string& deviceKey) const
{
    return {SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + deviceKey,
            SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey,
            SUBDEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
              DEVICE_PATH_PREFIX + deviceKey,
            SUBDEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey};
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
  const std::string& gatewayKey, const SubdeviceRegistrationRequest& request) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonPayload(request);
        const std::string channel = SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;

        auto r = jsonPayload.dump();

        return std::unique_ptr<Message>(new Message(r, channel));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Subdevice registration protocol: Unable to serialize subdevice registration request: "
                   << e.what();
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeMessage(
  const std::string& gatewayKey, const wolkabout::GatewayUpdateRequest& request) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonPayload(request);
        const auto channel = GATEWAY_UPDATE_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;

        return std::unique_ptr<Message>(new Message(jsonPayload.dump(), channel));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Subdevice registration protocol: Unable to serialize gateway update request: " << e.what();
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeSubdeviceDeletionRequestMessage(
  const std::string& gatewayKey, const std::string& deviceKey) const
{
    LOG(TRACE) << METHOD_INFO;

    std::stringstream channel;
    channel << SUBDEVICE_DELETION_REQUEST_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
    if (deviceKey != gatewayKey)
    {
        channel << CHANNEL_DELIMITER + DEVICE_PATH_PREFIX + deviceKey;
    }

    return std::unique_ptr<Message>(new Message("", channel.str()));
}

std::unique_ptr<SubdeviceRegistrationRequest>
JsonGatewaySubdeviceRegistrationProtocol::makeSubdeviceRegistrationRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonRequest = json::parse(message.getContent());
        SubdeviceRegistrationRequest request = subdevice_registration_request_from_json(jsonRequest);

        return std::unique_ptr<SubdeviceRegistrationRequest>(new SubdeviceRegistrationRequest(request));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Subdevice registration protocol: Unable to deserialize subdevice registration request: "
                   << e.what();
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

std::unique_ptr<SubdeviceRegistrationResponse>
JsonGatewaySubdeviceRegistrationProtocol::makeSubdeviceRegistrationResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonResponse = json::parse(message.getContent());
        SubdeviceRegistrationResponse response = subdevice_registration_response_from_json(jsonResponse);

        return std::unique_ptr<SubdeviceRegistrationResponse>(new SubdeviceRegistrationResponse(response));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Subdevice registration protocol: Unable to deserialize subdevice registration response: "
                   << e.what();
        return nullptr;
    }
}

std::unique_ptr<GatewayUpdateResponse> JsonGatewaySubdeviceRegistrationProtocol::makeGatewayUpdateResponse(
  const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const json jsonResponse = json::parse(message.getContent());
        GatewayUpdateResponse response = gateway_update_response_from_json(jsonResponse);

        return std::unique_ptr<GatewayUpdateResponse>(new GatewayUpdateResponse(response));
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "Subdevice registration protocol: Unable to deserialize subdevice registration response: "
                   << e.what();
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

bool JsonGatewaySubdeviceRegistrationProtocol::isSubdeviceRegistrationRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isSubdeviceRegistrationResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isSubdeviceDeletionRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), SUBDEVICE_DELETION_REQUEST_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isSubdeviceDeletionResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), SUBDEVICE_DELETION_RESPONSE_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isGatewayUpdateResponse(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), GATEWAY_UPDATE_RESPONSE_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isGatewayUpdateRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), GATEWAY_UPDATE_REQUEST_TOPIC_ROOT);
}

std::string JsonGatewaySubdeviceRegistrationProtocol::getResponseChannel(const Message& message,
                                                                         const std::string& gatewayKey,
                                                                         const std::string& deviceKey) const
{
    LOG(TRACE) << METHOD_INFO;

    if (isSubdeviceRegistrationRequest(message))
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
    else if (isSubdeviceDeletionRequest(message))
    {
        if (gatewayKey == deviceKey)
        {
            return SUBDEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
        }
        else
        {
            return SUBDEVICE_DELETION_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                   DEVICE_PATH_PREFIX + deviceKey;
        }
    }
    else if (isGatewayUpdateRequest(message))
    {
        return GATEWAY_UPDATE_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
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
