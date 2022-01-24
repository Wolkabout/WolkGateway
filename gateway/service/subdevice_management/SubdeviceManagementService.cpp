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

#include "gateway/service/subdevice_management/SubdeviceManagementService.h"

#include "core/connectivity/OutboundRetryMessageHandler.h"
#include "core/model/Message.h"
#include "core/model/messages/RegisteredDevicesRequestMessage.h"
#include "core/model/messages/RegisteredDevicesResponseMessage.h"
#include "core/protocol/GatewayRegistrationProtocol.h"
#include "core/protocol/RegistrationProtocol.h"
#include "core/utilities/Logger.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace
{
const std::uint16_t RETRY_COUNT = 3;
const std::chrono::milliseconds RETRY_TIMEOUT{5000};
}    // namespace

namespace wolkabout
{
namespace gateway
{
RegisteredDevicesRequestParameters::RegisteredDevicesRequestParameters(const std::chrono::milliseconds& timestampFrom,
                                                                       std::string deviceType, std::string externalId)
: m_timestampFrom(timestampFrom), m_deviceType(std::move(deviceType)), m_externalId(std::move(externalId))
{
}

const std::chrono::milliseconds& RegisteredDevicesRequestParameters::getTimestampFrom() const
{
    return m_timestampFrom;
}

const std::string& RegisteredDevicesRequestParameters::getDeviceType() const
{
    return m_deviceType;
}

const std::string& RegisteredDevicesRequestParameters::getExternalId() const
{
    return m_externalId;
}

bool RegisteredDevicesRequestParameters::operator==(const RegisteredDevicesRequestParameters& rvalue) const
{
    return m_timestampFrom == rvalue.m_timestampFrom && m_deviceType == rvalue.getDeviceType() &&
           m_externalId == rvalue.m_externalId;
}

std::uint64_t RegisteredDevicesRequestParametersHash::operator()(const RegisteredDevicesRequestParameters& params) const
{
    auto timestampHash = std::hash<std::uint64_t>()(static_cast<std::uint64_t>(params.getTimestampFrom().count()));
    auto deviceTypeHash = std::hash<std::string>()(params.getDeviceType());
    auto externalIdHash = std::hash<std::string>()(params.getExternalId());
    return timestampHash ^ deviceTypeHash ^ externalIdHash;
}

SubdeviceManagementService::SubdeviceManagementService(std::string gatewayKey,
                                                       RegistrationProtocol& platformRegistrationProtocol,
                                                       GatewayRegistrationProtocol& localRegistrationProtocol,
                                                       OutboundRetryMessageHandler& outboundPlatformMessageHandler,
                                                       OutboundMessageHandler& outboundDeviceMessageHandler,
                                                       DeviceRepository& deviceRepository)
: m_gatewayKey{std::move(gatewayKey)}
, m_platformProtocol{platformRegistrationProtocol}
, m_localProtocol{localRegistrationProtocol}
, m_outboundPlatformRetryMessageHandler{outboundPlatformMessageHandler}
, m_outboundLocalMessageHandler{outboundDeviceMessageHandler}
, m_deviceRepository{deviceRepository}

{
}

SubdeviceManagementService::~SubdeviceManagementService() = default;

void SubdeviceManagementService::messageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    // Pass it through to the retry manager
    m_outboundPlatformRetryMessageHandler.messageReceived(message);
}

const Protocol& SubdeviceManagementService::getProtocol()
{
    return m_localProtocol;
}

void SubdeviceManagementService::receiveMessages(const std::vector<GatewaySubdeviceMessage>& messages)
{
    LOG(TRACE) << METHOD_INFO;

    // Go through every message
    for (const auto& message : messages)
    {
        // Try to parse the message
        const auto type = m_platformProtocol.getMessageType(message.getMessage());
        if (type != MessageType::REGISTERED_DEVICES_RESPONSE)
        {
            LOG(WARN) << "Received message that is not 'RegisteredDevicesResponse' message. Ignoring...";
            return;
        }
        const auto sharedMessage = std::make_shared<Message>(message.getMessage());
        const auto response = m_platformProtocol.parseRegisteredDevicesResponse(sharedMessage);
        if (response == nullptr)
        {
            LOG(ERROR) << "Failed to parse incoming 'RegisteredDevicesResponse' message.";
            return;
        }

        // Print something about it
        LOG(INFO) << "Received info about " << response->getMatchingDevices().size() << " devices!";
    }
}

std::vector<MessageType> SubdeviceManagementService::getMessageTypes()
{
    return {MessageType::REGISTERED_DEVICES_RESPONSE};
}

bool SubdeviceManagementService::sendOutRegisteredDevicesRequest(std::chrono::milliseconds timestampFrom,
                                                                 const std::string& deviceType,
                                                                 const std::string& externalId)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to send out a 'RegisteredDevicesRequest' message -> ";

    // Form the message
    auto message = RegisteredDevicesRequestMessage{timestampFrom, deviceType, externalId};

    // Parse the message
    auto parsedMessage = std::shared_ptr<Message>{m_platformProtocol.makeOutboundMessage(m_gatewayKey, message)};
    if (parsedMessage == nullptr)
    {
        LOG(ERROR) << errorPrefix << "Failed to parse the outbound message.";
        return false;
    }

    // Send it out
    m_outboundPlatformRetryMessageHandler.addMessage(RetryMessageStruct{
      parsedMessage, m_platformProtocol.getResponseChannelForRegisteredDeviceRequest(m_gatewayKey),
      [=](const std::shared_ptr<Message>&)
      { LOG(ERROR) << "Failed to receive response for 'RegisteredDevicesRequest' - no response from platform."; },
      RETRY_COUNT, RETRY_TIMEOUT});
    auto params = RegisteredDevicesRequestParameters{timestampFrom, deviceType, externalId};
    m_requests.emplace(params, [](std::unique_ptr<RegisteredDevicesResponseMessage>) {});
    return true;
}
}    // namespace gateway
}    // namespace wolkabout
