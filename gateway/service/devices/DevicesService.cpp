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

#include <algorithm>
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

RegisteredDevicesRequestCallback::RegisteredDevicesRequestCallback(
  std::weak_ptr<std::condition_variable> conditionVariable)
: m_conditionVariable{std::move(conditionVariable)}
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

const std::weak_ptr<std::condition_variable>& RegisteredDevicesRequestCallback::getConditionVariable() const
{
    return m_conditionVariable;
}

ChildrenSynchronizationRequestCallback::ChildrenSynchronizationRequestCallback(
  std::function<void(std::unique_ptr<ChildrenSynchronizationResponseMessage>)> lambda,
  std::vector<std::string> registeringDevices)
: m_registeringDevices{std::move(registeringDevices)}, m_lambda{std::move(lambda)}
{
}

ChildrenSynchronizationRequestCallback::ChildrenSynchronizationRequestCallback(
  std::weak_ptr<std::condition_variable> conditionVariable, std::vector<std::string> registeringDevices)
: m_registeringDevices{std::move(registeringDevices)}, m_conditionVariable{std::move(conditionVariable)}
{
}

const std::chrono::milliseconds& ChildrenSynchronizationRequestCallback::getSentTime() const
{
    return m_sentTime;
}

const std::vector<std::string>& ChildrenSynchronizationRequestCallback::getRegisteringDevices() const
{
    return m_registeringDevices;
}

const std::function<void(std::unique_ptr<ChildrenSynchronizationResponseMessage>)>&
ChildrenSynchronizationRequestCallback::getLambda() const
{
    return m_lambda;
}
const std::weak_ptr<std::condition_variable>& ChildrenSynchronizationRequestCallback::getConditionVariable() const
{
    return m_conditionVariable;
}

DevicesService::DevicesService(std::string gatewayKey, RegistrationProtocol& platformRegistrationProtocol,
                               OutboundMessageHandler& outboundPlatformMessageHandler,
                               OutboundRetryMessageHandler& outboundPlatformRetryMessageHandler,
                               std::shared_ptr<GatewayRegistrationProtocol> localRegistrationProtocol,
                               std::shared_ptr<OutboundMessageHandler> outboundDeviceMessageHandler,
                               std::shared_ptr<DeviceRepository> deviceRepository,
                               std::shared_ptr<ExistingDevicesRepository> existingDevicesRepository)
: m_gatewayKey{std::move(gatewayKey)}
, m_platformProtocol{platformRegistrationProtocol}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundPlatformRetryMessageHandler{outboundPlatformRetryMessageHandler}
, m_localProtocol{std::move(localRegistrationProtocol)}
, m_outboundLocalMessageHandler{std::move(outboundDeviceMessageHandler)}
, m_deviceRepository{std::move(deviceRepository)}
, m_existingDeviceRepository{std::move(existingDevicesRepository)}
{
}

DevicesService::~DevicesService() = default;

bool DevicesService::registerChildDevices(
  const std::vector<DeviceRegistrationData>& devices,
  const std::function<void(const std::vector<std::string>&, const std::vector<std::string>&)>& callback)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to register child devices -> ";

    // Form the message for the registration
    const auto parsedMessage = std::shared_ptr<Message>{
      m_platformProtocol.makeOutboundMessage(m_gatewayKey, DeviceRegistrationMessage{devices})};
    if (parsedMessage == nullptr)
    {
        LOG(ERROR) << errorPrefix << "Failed to parse the outbound 'DeviceRegistrationMessage'.";
        return false;
    }

    // Publish the message
    m_outboundPlatformMessageHandler.addMessage(parsedMessage);

    // Now that that's publish, we want to verify that with the ChildrenSynchronizationMessage
    auto deviceKeys = std::vector<std::string>{};
    for (const auto& device : devices)
        deviceKeys.emplace_back(device.key);
    sendOutChildrenSynchronizationRequest(std::make_shared<ChildrenSynchronizationRequestCallback>(
      [=](const std::unique_ptr<ChildrenSynchronizationResponseMessage>& response) {
          // Contains all devices
          auto succeeded = std::vector<std::string>{};
          auto failed = std::vector<std::string>{};

          // Check the devices
          const auto& children = response != nullptr ? response->getChildren() : std::vector<std::string>{};
          for (const auto& device : deviceKeys)
          {
              const auto it = std::find(children.cbegin(), children.cend(), device);
              if (it != children.cend())
                  succeeded.emplace_back(device);
              else
                  failed.emplace_back(device);
          }
          callback(succeeded, failed);
      },
      deviceKeys));
    return true;
}

bool DevicesService::removeChildDevices(const std::vector<std::string>& deviceKeys)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to remove child devices -> ";

    // Form the message for the registration
    const auto parsedMessage =
      std::shared_ptr<Message>{m_platformProtocol.makeOutboundMessage(m_gatewayKey, DeviceRemovalMessage{deviceKeys})};
    if (parsedMessage == nullptr)
    {
        LOG(ERROR) << errorPrefix << "Failed to parse the outbound 'DeviceRemovalMessage'.";
        return false;
    }

    // Publish the message
    m_outboundPlatformMessageHandler.addMessage(parsedMessage);
    return true;
}

void DevicesService::updateDeviceCache()
{
    LOG(TRACE) << METHOD_INFO;

    // Check whether a device repository even exists
    if (m_deviceRepository == nullptr)
    {
        LOG(WARN) << "Skipping update device cache - no device repository exists...";
        return;
    }
    // If we have an existing device repository, we want to check whether the user wants any devices deleted.
    if (m_existingDeviceRepository != nullptr)
    {
        const auto gatewayDevices = m_deviceRepository->getGatewayDevices();
        const auto keys = m_existingDeviceRepository->getDeviceKeys();
        auto toDelete = std::vector<std::string>{};
        for (const auto& gatewayDevice : gatewayDevices)
        {
            const auto it = std::find(keys.cbegin(), keys.cend(), gatewayDevice.getDeviceKey());
            if (it == keys.cend())
                toDelete.emplace_back(gatewayDevice.getDeviceKey());
        }
        if (removeChildDevices(toDelete))
            m_deviceRepository->remove(toDelete);
        else
            LOG(ERROR) << "Failed to send out a 'DeviceRemoval' request to remove devices deleted from "
                          "'ExistingDeviceRepository'.";
    }

    // Obtain the last timestamp and send out a request
    auto lastTimestamp = m_deviceRepository->latestPlatformTimestamp();
    LOG(DEBUG) << TAG << "Obtaining devices from timestamp " << lastTimestamp.count() << ".";
    sendOutRegisteredDevicesRequest(RegisteredDevicesRequestParameters{lastTimestamp}, {});
    sendOutChildrenSynchronizationRequest({});
}

bool DevicesService::sendOutChildrenSynchronizationRequest(
  std::shared_ptr<ChildrenSynchronizationRequestCallback> callback)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to send out a 'ChildrenSynchronizationRequestMessage' -> ";

    // Form the message
    auto parsedMessage = std::shared_ptr<Message>{
      m_platformProtocol.makeOutboundMessage(m_gatewayKey, ChildrenSynchronizationRequestMessage{})};
    if (parsedMessage == nullptr)
    {
        LOG(ERROR) << errorPrefix << "Failed to parse the outbound message.";
        return false;
    }
    m_outboundPlatformRetryMessageHandler.addMessage(RetryMessageStruct{
      parsedMessage,
      m_platformProtocol.getResponseChannelForMessage(MessageType::CHILDREN_SYNCHRONIZATION_REQUEST, m_gatewayKey),
      [=](const std::shared_ptr<Message>&) {
          LOG(ERROR)
            << TAG
            << "Failed to receive response for 'ChildrenSynchronizationRequestMessage' - no response from platform.";
          // Check the callback
          if (callback != nullptr)
          {
              if (!callback->getRegisteringDevices().empty())
              {
                  LOG(ERROR) << "Failed to register devices: ";
                  for (const auto& device : callback->getRegisteringDevices())
                      LOG(ERROR) << "\t" << device;
              }
              if (auto cv = callback->getConditionVariable().lock())
                  cv->notify_one();
              if (callback->getLambda())
                  callback->getLambda()(nullptr);
          }
      },
      RETRY_COUNT, RETRY_TIMEOUT});
    {
        std::lock_guard<std::mutex> lock{m_childSyncMutex};
        if (callback != nullptr)
            m_childSyncRequests.push(std::move(callback));
    }
    return true;
}

bool DevicesService::sendOutRegisteredDevicesRequest(RegisteredDevicesRequestParameters parameters,
                                                     std::shared_ptr<RegisteredDevicesRequestCallback> callback)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to send out a 'RegisteredDevicesRequestMessage' -> ";

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
      parsedMessage,
      m_platformProtocol.getResponseChannelForMessage(MessageType::REGISTERED_DEVICES_REQUEST, m_gatewayKey),
      [=](const std::shared_ptr<Message>&) {
          LOG(ERROR) << TAG << "Failed to receive response for 'RegisteredDevicesRequest' - no response from platform.";
      },
      RETRY_COUNT, RETRY_TIMEOUT});
    m_registeredDevicesRequests.emplace(std::move(parameters), std::move(callback));
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

        // Send the message
        registerChildDevices(parsedMessage->getDevices(), [=](const std::vector<std::string>& registeredDevices,
                                                              const std::vector<std::string>& unregisteredDevices) {
            if (m_localProtocol == nullptr)
                return;
            auto responseMessage = std::shared_ptr<Message>{m_localProtocol->makeOutboundMessage(
              deviceKey, DeviceRegistrationResponseMessage{registeredDevices, unregisteredDevices})};
            if (responseMessage == nullptr)
                return;
            m_outboundLocalMessageHandler->addMessage(responseMessage);
        });
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
        auto callback = std::shared_ptr<RegisteredDevicesRequestCallback>{};
        if (m_localProtocol != nullptr && m_outboundLocalMessageHandler != nullptr)
        {
            callback = std::make_shared<RegisteredDevicesRequestCallback>(
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
              });
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
        // Give the message to the RetryMessageHandler
        const auto sharedMessage = std::make_shared<Message>(message.getMessage());
        m_outboundPlatformRetryMessageHandler.messageReceived(sharedMessage);

        // Try to parse the message
        const auto type = m_platformProtocol.getMessageType(message.getMessage());
        switch (type)
        {
        case MessageType::CHILDREN_SYNCHRONIZATION_RESPONSE:
        {
            auto response = m_platformProtocol.parseChildrenSynchronizationResponse(sharedMessage);
            if (response == nullptr)
                LOG(ERROR) << TAG << "Failed to parse incoming 'ChildrenSynchronizationResponseMessage'.";
            else
                handleChildrenSynchronizationResponse(std::move(response));
            break;
        }
        case MessageType::REGISTERED_DEVICES_RESPONSE:
        {
            auto response = m_platformProtocol.parseRegisteredDevicesResponse(sharedMessage);
            if (response == nullptr)
                LOG(ERROR) << TAG << "Failed to parse incoming 'RegisteredDevicesResponseMessage'.";
            else
                handleRegisteredDevicesResponse(std::move(response));
            break;
        }
        default:
            LOG(WARN) << TAG << "Received message is of type that can not be handled. Ignoring...";
            break;
        }
    }
}

std::vector<MessageType> DevicesService::getMessageTypes()
{
    return {MessageType::CHILDREN_SYNCHRONIZATION_RESPONSE, MessageType::REGISTERED_DEVICES_RESPONSE};
}

bool DevicesService::deviceExists(const std::string& deviceKey)
{
    return m_deviceRepository == nullptr || m_deviceRepository->containsDevice(deviceKey);
}

void DevicesService::handleChildrenSynchronizationResponse(
  std::unique_ptr<ChildrenSynchronizationResponseMessage> response)
{
    LOG(TRACE) << METHOD_INFO;

    // Check if any callbacks are waiting
    auto callback = std::shared_ptr<ChildrenSynchronizationRequestCallback>{};
    {
        std::lock_guard<std::mutex> lock{m_childSyncMutex};
        if (!m_childSyncRequests.empty())
        {
            callback = m_childSyncRequests.front();
            m_childSyncRequests.pop();
        }
    }

    // Add the devices to storage
    LOG(INFO) << TAG << "Received info about " << response->getChildren().size() << " child devices!.";
    if (m_deviceRepository != nullptr)
    {
        auto devicesToSave = std::vector<StoredDeviceInformation>{};
        for (const auto& device : response->getChildren())
            devicesToSave.emplace_back(
              StoredDeviceInformation{device, DeviceOwnership::Gateway, std::chrono::milliseconds{0}});
        m_deviceRepository->save(devicesToSave);
    }
    if (m_existingDeviceRepository != nullptr)
    {
        auto savedDevices = m_existingDeviceRepository->getDeviceKeys();
        for (const auto& device : response->getChildren())
            if (std::find(savedDevices.cbegin(), savedDevices.cend(), device) == savedDevices.cend())
                m_existingDeviceRepository->addDeviceKey(device);
    }

    // Handle the callback
    if (callback != nullptr)
    {
        if (auto cv = callback->getConditionVariable().lock())
            cv->notify_one();
        else if (callback->getLambda())
            callback->getLambda()(std::move(response));
    }
}

void DevicesService::handleRegisteredDevicesResponse(std::unique_ptr<RegisteredDevicesResponseMessage> response)
{
    LOG(TRACE) << METHOD_INFO;

    // Look for the callback object
    auto now =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    auto callback = std::shared_ptr<RegisteredDevicesRequestCallback>{};
    const auto params = RegisteredDevicesRequestParameters{response->getTimestampFrom(), response->getDeviceType(),
                                                           response->getExternalId()};
    {
        std::lock_guard<std::mutex> lock{m_registeredDevicesMutex};
        const auto callbackIt = m_registeredDevicesRequests.find(params);
        if (callbackIt != m_registeredDevicesRequests.cend() && callbackIt->second != nullptr)
        {
            // Update the time when the request was sent out
            callback = callbackIt->second;
            now = callback->getSentTime();
        }
    }

    // Print something about it
    LOG(INFO) << TAG << "Received info about " << response->getMatchingDevices().size() << " roaming devices!";
    if (m_deviceRepository != nullptr)
    {
        auto devicesToSave = std::vector<StoredDeviceInformation>{};
        if (m_deviceRepository != nullptr)
            for (const auto& device : response->getMatchingDevices())
                devicesToSave.emplace_back(StoredDeviceInformation{device, now});
        m_deviceRepository->save(devicesToSave);
    }

    // Handle the callback
    if (callback != nullptr)
    {
        if (auto cv = callback->getConditionVariable().lock())
            cv->notify_one();
        else if (callback->getLambda())
            callback->getLambda()(std::move(response));
    }
}
}    // namespace gateway
}    // namespace wolkabout
