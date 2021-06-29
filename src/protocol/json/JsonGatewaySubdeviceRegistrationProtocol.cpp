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

#include "core/model/Message.h"
#include "core/model/SubdeviceRegistrationRequest.h"
#include "core/model/SubdeviceRegistrationResponse.h"
#include "core/model/SubdeviceUpdateRequest.h"
#include "core/model/SubdeviceUpdateResponse.h"
#include "core/protocol/json/Json.h"
#include "core/protocol/json/JsonDto.h"
#include "core/utilities/Logger.h"
#include "core/utilities/StringUtils.h"
#include "core/utilities/json.hpp"

#include <stdexcept>

using nlohmann::json;

namespace wolkabout
{
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT =
  "d2p/register_subdevice_request/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT =
  "p2d/register_subdevice_response/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_UPDATE_REQUEST_TOPIC_ROOT =
  "d2p/update_subdevice_request/";
const std::string JsonGatewaySubdeviceRegistrationProtocol::SUBDEVICE_UPDATE_RESPONSE_TOPIC_ROOT =
  "p2d/update_subdevice_response/";

std::vector<std::string> JsonGatewaySubdeviceRegistrationProtocol::getInboundChannels() const
{
    return {SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewaySubdeviceRegistrationProtocol::getInboundChannelsForDevice(
  const std::string& deviceKey) const
{
    return {SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey};
}

std::unique_ptr<SubdeviceRegistrationRequest>
JsonGatewaySubdeviceRegistrationProtocol::makeSubdeviceRegistrationRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    if (!StringUtils::startsWith(message.getChannel(), SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT))
    {
        return nullptr;
    }

    try
    {
        const json jsonRequest = json::parse(message.getContent());
        SubdeviceRegistrationRequest request = subdevice_registration_request_from_json(jsonRequest);

        return std::unique_ptr<SubdeviceRegistrationRequest>(new SubdeviceRegistrationRequest(request));
    }
    catch (std::exception& e)
    {
        LOG(DEBUG) << "Gateway subdevice registration protocol: Unable to deserialize subdevice registration request: "
                   << e.what();
        return nullptr;
    }
    catch (...)
    {
        LOG(DEBUG) << "Gateway subdevice registration protocol: Unable to deserialize subdevice registration request";
        return nullptr;
    }
}

std::unique_ptr<SubdeviceUpdateRequest> JsonGatewaySubdeviceRegistrationProtocol::makeSubdeviceUpdateRequest(
  const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    if (!StringUtils::startsWith(message.getChannel(), SUBDEVICE_UPDATE_REQUEST_TOPIC_ROOT))
    {
        return nullptr;
    }

    auto deviceKey = extractDeviceKeyFromChannel(message.getChannel());

    if (deviceKey.empty())
    {
        LOG(DEBUG) << "Gateway subdevice registration protocol: Unable to extract key from subdevice update request";
        return nullptr;
    }

    try
    {
        const json jsonRequest = json::parse(message.getContent());
        SubdeviceUpdateRequest request = subdevice_update_request_from_json(jsonRequest, deviceKey);

        return std::unique_ptr<SubdeviceUpdateRequest>(new SubdeviceUpdateRequest(request));
    }
    catch (std::exception& e)
    {
        LOG(DEBUG) << "Gateway subdevice registration protocol: Unable to deserialize subdevice registration request: "
                   << e.what();
        return nullptr;
    }
    catch (...)
    {
        LOG(DEBUG) << "Gateway subdevice registration protocol: Unable to deserialize subdevice registration request";
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeMessage(
  const wolkabout::SubdeviceRegistrationResponse& response) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const auto channel =
          SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + response.getSubdeviceKey();

        const json jsonPayload(response);
        const auto content = jsonPayload.dump();

        return std::unique_ptr<Message>(new Message(content, channel));
    }
    catch (std::exception& e)
    {
        LOG(DEBUG) << "Gateway subdevice registration protocol: Unable to serialize device registration response: "
                   << e.what();
        return nullptr;
    }
    catch (...)
    {
        LOG(DEBUG) << "Gateway subdevice registration protocol: Unable to serialize device registration response";
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewaySubdeviceRegistrationProtocol::makeMessage(
  const SubdeviceUpdateResponse& response) const
{
    LOG(TRACE) << METHOD_INFO;

    try
    {
        const auto channel = SUBDEVICE_UPDATE_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + response.getSubdeviceKey();

        const json jsonPayload(response);
        const auto content = jsonPayload.dump();

        return std::unique_ptr<Message>(new Message(content, channel));
    }
    catch (std::exception& e)
    {
        LOG(DEBUG) << "Gateway subdevice registration protocol: Unable to serialize device registration response: "
                   << e.what();
        return nullptr;
    }
    catch (...)
    {
        LOG(DEBUG) << "Gateway subdevice registration protocol: Unable to serialize device registration response";
        return nullptr;
    }
}

bool JsonGatewaySubdeviceRegistrationProtocol::isSubdeviceRegistrationRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT);
}

bool JsonGatewaySubdeviceRegistrationProtocol::isSubdeviceUpdateRequest(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), SUBDEVICE_UPDATE_REQUEST_TOPIC_ROOT);
}
}    // namespace wolkabout
