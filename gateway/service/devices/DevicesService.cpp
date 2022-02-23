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

#include "gateway/service/devices/DevicesService.h"

#include "core/connectivity/OutboundMessageHandler.h"
#include "core/connectivity/OutboundRetryMessageHandler.h"
#include "core/model/Message.h"
#include "core/model/messages/RegisteredDevicesRequestMessage.h"
#include "core/model/messages/RegisteredDevicesResponseMessage.h"
#include "core/protocol/GatewayRegistrationProtocol.h"
#include "core/protocol/RegistrationProtocol.h"
#include "core/utilities/Logger.h"
#include "gateway/repository/device/DeviceRepository.h"
#include "gateway/repository/existing_device/ExistingDevicesRepository.h"

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

RegisteredDevicesRequestCallback::RegisteredDevicesRequestCallback(
  std::function<void(std::unique_ptr<RegisteredDevicesResponseMessage>)> lambda)
: m_lambda{std::move(lambda)}
{
}

const std::chrono::milliseconds& RegisteredDevicesRequestCallback::getSentTime() const
{
    return m_sentTime;
}

const std::function<void(std::unique_ptr<RegisteredDevicesResponseMessage>)>&
RegisteredDevicesRequestCallback::getLambda() const
{
    return m_lambda;
}

DevicesService::DevicesService(std::string gatewayKey, RegistrationProtocol& platformRegistrationProtocol,
                               OutboundMessageHandler& outboundPlatformMessageHandler,
                               OutboundRetryMessageHandler& outboundPlatformRetryMessageHandler,
                               std::shared_ptr<GatewayRegistrationProtocol> localRegistrationProtocol,
                               std::shared_ptr<OutboundMessageHandler> outboundDeviceMessageHandler,
                               std::shared_ptr<DeviceRepository> deviceRepository)
: m_gatewayKey{std::move(gatewayKey)}
, m_platformProtocol{platformRegistrationProtocol}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundPlatformRetryMessageHandler{outboundPlatformRetryMessageHandler}
, m_localProtocol{std::move(localRegistrationProtocol)}
, m_outboundLocalMessageHandler{std::move(outboundDeviceMessageHandler)}
, m_deviceRepository{std::move(deviceRepository)}
{
}

DevicesService::~DevicesService() = default;

void DevicesService::updateDeviceCache()
{
    LOG(TRACE) << METHOD_INFO;

    // Check whether a device repository even exists
    if (m_deviceRepository == nullptr)
    {
        LOG(WARN) << "Skipping update device cache - no device repository exists...";
        return;
    }

    // Obtain the last timestamp and send out a request
    auto lastTimestamp = m_deviceRepository->latestTimestamp();
    LOG(DEBUG) << TAG << "Obtaining devices from timestamp " << lastTimestamp.count() << ".";
    sendOutRegisteredDevicesRequest(RegisteredDevicesRequestParameters{lastTimestamp}, {});
}

bool DevicesService::sendOutRegisteredDevicesRequest(RegisteredDevicesRequestParameters parameters,
                                                     RegisteredDevicesRequestCallback callback)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to send out a 'RegisteredDevicesRequest' message -> ";

    // Form the message
    auto message = RegisteredDevicesRequestMessage{parameters.getTimestampFrom(), parameters.getDeviceType(),
                                                   parameters.getExternalId()};

    // Parse the message
    auto parsedMessage = std::shared_ptr<Message>{m_platformProtocol.makeOutboundMessage(m_gatewayKey, message)};
    if (parsedMessage == nullptr)
    {
        LOG(ERROR) << TAG << errorPrefix << "Failed to parse the outbound message.";
        return false;
    }

    // Send it out
    auto sendTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    m_outboundPlatformRetryMessageHandler.addMessage(RetryMessageStruct{
      parsedMessage, m_platformProtocol.getResponseChannelForRegisteredDeviceRequest(m_gatewayKey),
      [=](const std::shared_ptr<Message>&) {
          LOG(ERROR) << TAG << "Failed to receive response for 'RegisteredDevicesRequest' - no response from platform.";
      },
      RETRY_COUNT, RETRY_TIMEOUT});
    m_requests.emplace(std::move(parameters), std::move(callback));
    return true;
}

void DevicesService::messageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    // Check the protocol
    if (m_localProtocol == nullptr)
    {
        LOG(ERROR) << "Received incoming message but local protocol is missing.";
        return;
    }

    // Figure out the message type and the key of the device that has sent it
    auto messageType = m_localProtocol->getMessageType(*message);
    auto deviceKey = m_localProtocol->getDeviceKey(*message);
    switch (messageType)
    {
    case MessageType::DEVICE_REGISTRATION:
    {
        // Route the message to the platform
        auto parsedMessage = m_localProtocol->parseDeviceRegistrationMessage(message);
        if (parsedMessage == nullptr)
        {
            LOG(ERROR) << TAG
                       << "Failed to handle incoming local 'DeviceRegistration' message - Failed to parse the message.";
            return;
        }

        // Make the platform request
        auto request = std::shared_ptr<Message>{m_platformProtocol.makeOutboundMessage(m_gatewayKey, *parsedMessage)};
        if (request == nullptr)
        {
            LOG(ERROR) << TAG
                       << "Failed to handler incoming local 'DeviceRegistration' message - Failed to parse outgoing "
                          "registration request.";
            return;
        }

        // Send the message
        m_outboundPlatformMessageHandler.addMessage(request);
    }
    case MessageType::DEVICE_REMOVAL:
    {
        // Route the message to the platform
        auto parsedMessage = m_localProtocol->parseDeviceRemovalMessage(message);
        if (parsedMessage == nullptr)
        {
            LOG(ERROR) << TAG
                       << "Failed to handle incoming local 'DeviceRemoval' message - Failed to parse the message.";
            return;
        }

        // Make the platform request
        auto request = std::shared_ptr<Message>{m_platformProtocol.makeOutboundMessage(m_gatewayKey, *parsedMessage)};
        if (request == nullptr)
        {
            LOG(ERROR) << TAG
                       << "Failed to handler incoming local 'DeviceRemoval' message - Failed to parse outgoing "
                          "registration request.";
            return;
        }

        // Send the message
        m_outboundPlatformMessageHandler.addMessage(request);
    }
    case MessageType::REGISTERED_DEVICES_REQUEST:
    {
        // This message needs to be routed in such a way, that the response for this message will be routed back on the
        // local broker
        // First parse the message
        auto parsedMessage = m_localProtocol->parseRegisteredDevicesRequestMessage(message);
        if (parsedMessage == nullptr)
        {
            LOG(ERROR)
              << TAG
              << "Failed to handle incoming local 'RegisteredDevicesRequest' message - Failed to parse the message.";
            return;
        }

        // Make the message for the platform request
        auto request = RegisteredDevicesRequestParameters{
          parsedMessage->getTimestampFrom(), parsedMessage->getDeviceType(), parsedMessage->getExternalId()};

        // Create the callback
        auto callback = RegisteredDevicesRequestCallback{};
        if (m_localProtocol != nullptr && m_outboundLocalMessageHandler != nullptr)
        {
            callback = RegisteredDevicesRequestCallback{
              [this, deviceKey](std::unique_ptr<RegisteredDevicesResponseMessage> response) {
                  // Create the message for the local broker
                  auto localResponse =
                    std::shared_ptr<Message>{m_localProtocol->makeOutboundMessage(deviceKey, *response)};
                  if (localResponse == nullptr)
                  {
                      LOG(ERROR) << TAG
                                 << "Failed to parse outgoing response for local 'RegisteredDevicesRequest' message.";
                      return;
                  }
                  m_outboundLocalMessageHandler->addMessage(localResponse);
              }};
        }

        // Send out the request
        sendOutRegisteredDevicesRequest(request, callback);
        break;
    }
    default:
        LOG(WARN) << TAG << "Received message of invalid type.";
    }
}

const Protocol& DevicesService::getProtocol()
{
    if (m_localProtocol == nullptr)
        throw std::runtime_error("Request protocol from an object where local communication is disabled.");
    return *m_localProtocol;
}

void DevicesService::receiveMessages(const std::vector<GatewaySubdeviceMessage>& messages)
{
    LOG(TRACE) << METHOD_INFO;

    // Go through every message
    for (const auto& message : messages)
    {
        // Try to parse the message
        const auto type = m_platformProtocol.getMessageType(message.getMessage());
        if (type != MessageType::REGISTERED_DEVICES_RESPONSE)
        {
            LOG(WARN) << TAG << "Received message that is not 'RegisteredDevicesResponse' message. Ignoring...";
            return;
        }
        const auto sharedMessage = std::make_shared<Message>(message.getMessage());
        m_outboundPlatformRetryMessageHandler.messageReceived(sharedMessage);
        auto response = m_platformProtocol.parseRegisteredDevicesResponse(sharedMessage);
        if (response == nullptr)
        {
            LOG(ERROR) << TAG << "Failed to parse incoming 'RegisteredDevicesResponse' message.";
            return;
        }

        // Look for the callback object
        auto now =
          std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        const auto params = RegisteredDevicesRequestParameters{response->getTimestampFrom(), response->getDeviceType(),
                                                               response->getExternalId()};
        const auto callbackIt = m_requests.find(params);
        if (callbackIt != m_requests.cend())
        {
            // Update the time when the request was sent out
            now = callbackIt->second.getSentTime();
        }

        // Print something about it
        LOG(INFO) << TAG << "Received info about " << response->getMatchingDevices().size() << " devices!";
        if (m_deviceRepository != nullptr)
            for (const auto& device : response->getMatchingDevices())
                m_deviceRepository->save(now, device);

        // Handle the callback
        if (callbackIt != m_requests.cend())
        {
            const auto& callback = callbackIt->second;
            if (callback.getLambda())
                callback.getLambda()(std::move(response));
        }
    }
}

std::vector<MessageType> DevicesService::getMessageTypes() const
{
    return {MessageType::REGISTERED_DEVICES_RESPONSE};
}
}    // namespace gateway
}    // namespace wolkabout
