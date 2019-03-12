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

#include "service/SubdeviceRegistrationService.h"
#include "OutboundMessageHandler.h"
#include "model/DetailedDevice.h"
#include "model/Message.h"
#include "model/SubdeviceRegistrationRequest.h"
#include "model/SubdeviceRegistrationResponse.h"
#include "protocol/GatewaySubdeviceRegistrationProtocol.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace
{
static const short RETRY_COUNT = 3;
static const std::chrono::milliseconds RETRY_TIMEOUT{5000};
}    // namespace

namespace wolkabout
{
SubdeviceRegistrationService::SubdeviceRegistrationService(std::string gatewayKey,
                                                           GatewaySubdeviceRegistrationProtocol& protocol,
                                                           DeviceRepository& deviceRepository,
                                                           OutboundMessageHandler& outboundPlatformMessageHandler,
                                                           OutboundMessageHandler& outboundDeviceMessageHandler)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_deviceRepository{deviceRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_platformRetryMessageHandler{outboundPlatformMessageHandler}
{
}

void SubdeviceRegistrationService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    if (!m_protocol.isMessageFromPlatform(*message))
    {
        LOG(WARN) << "SubdeviceRegistrationService: Ignoring message on channel '" << message->getChannel()
                  << "'. Message not from platform.";
        return;
    }

    m_platformRetryMessageHandler.messageReceived(message);

    if (m_protocol.isSubdeviceRegistrationResponse(*message))
    {
        const auto response = m_protocol.makeSubdeviceRegistrationResponse(*message);
        if (!response)
        {
            LOG(ERROR)
              << "SubdeviceRegistrationService: Device registration response could not be deserialized. Channel: '"
              << message->getChannel() << "' Payload: '" << message->getContent() << "'";
            return;
        }

        auto deviceKey = m_protocol.extractDeviceKeyFromChannel(message->getChannel());
        handleSubdeviceRegistrationResponse(deviceKey, *response);
    }
    else if (m_protocol.isSubdeviceDeletionResponse(*message))
    {
        LOG(INFO) << "SubdeviceRegistrationService: Received subdevice deletion response (" << message->getChannel()
                  << ")";
    }
    else
    {
        LOG(WARN) << "SubdeviceRegistrationService: Unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
    }
}

void SubdeviceRegistrationService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    if (!m_protocol.isMessageToPlatform(*message))
    {
        LOG(WARN) << "SubdeviceRegistrationService: Ignoring message received on channel '" << message->getChannel()
                  << "'. Message not intended for platform.";
        return;
    }

    if (!m_protocol.isSubdeviceRegistrationRequest(*message))
    {
        LOG(WARN) << "SubdeviceRegistrationService: unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
        return;
    }

    auto request = m_protocol.makeSubdeviceRegistrationRequest(*message);
    if (!request)
    {
        LOG(ERROR)
          << "SubdeviceRegistrationService: Subdevice registration request could not be deserialized. Channel: '"
          << message->getChannel() << "' Payload: '" << message->getContent() << "'";
        return;
    }

    auto deviceKey = m_protocol.extractDeviceKeyFromChannel(message->getChannel());
    if (!m_deviceRepository.containsDeviceWithKey(m_gatewayKey) && deviceKey != m_gatewayKey)
    {
        addToPostponedSubdeviceRegistrationRequests(deviceKey, *request);
        return;
    }

    handleSubdeviceRegistrationRequest(deviceKey, *request);
}

const GatewayProtocol& SubdeviceRegistrationService::getProtocol() const
{
    return m_protocol;
}

void SubdeviceRegistrationService::onDeviceRegistered(
  std::function<void(const std::string& deviceKey)> onDeviceRegistered)
{
    m_onDeviceRegistered = onDeviceRegistered;
}

void SubdeviceRegistrationService::invokeOnDeviceRegisteredListener(const std::string& deviceKey) const
{
    if (m_onDeviceRegistered)
    {
        m_onDeviceRegistered(deviceKey);
    }
}

void SubdeviceRegistrationService::deleteDevicesOtherThan(const std::vector<std::string>& devicesKeys)
{
    const auto deviceKeysFromRepository = m_deviceRepository.findAllDeviceKeys();
    for (const std::string& deviceKeyFromRepository : *deviceKeysFromRepository)
    {
        if (std::find(devicesKeys.begin(), devicesKeys.end(), deviceKeyFromRepository) == devicesKeys.end())
        {
            if (deviceKeyFromRepository == m_gatewayKey)
            {
                LOG(DEBUG) << "Skiping delete gateway";
                continue;
            }

            LOG(INFO) << "Deleting device with key " << deviceKeyFromRepository;
            m_deviceRepository.remove(deviceKeyFromRepository);

            std::shared_ptr<Message> subdeviceDeletionRequestMessage =
              m_protocol.makeSubdeviceDeletionRequestMessage(m_gatewayKey, deviceKeyFromRepository);
            if (!subdeviceDeletionRequestMessage)
            {
                LOG(WARN) << "SubdeviceRegistrationService: Unable to create deletion request message";
                continue;
            }

            auto responseChannel =
              m_protocol.getResponseChannel(*subdeviceDeletionRequestMessage, m_gatewayKey, deviceKeyFromRepository);
            RetryMessageStruct retryMessage{subdeviceDeletionRequestMessage, responseChannel,
                                            [=](std::shared_ptr<Message>) {
                                                LOG(ERROR)
                                                  << "Failed to delete device with key: " << deviceKeyFromRepository
                                                  << ", no response from platform";
                                            },
                                            RETRY_COUNT, RETRY_TIMEOUT};
            m_platformRetryMessageHandler.addMessage(retryMessage);
        }
    }
}

void SubdeviceRegistrationService::registerPostponedDevices()
{
    std::lock_guard<decltype(m_devicesWithPostponedRegistrationMutex)> devicesWithPostponedRegistrationLock(
      m_devicesWithPostponedRegistrationMutex);
    if (!m_devicesWithPostponedRegistration.empty())
    {
        LOG(INFO) << "SubdeviceRegistrationService: Processing postponed device registration requests";

        for (const auto& deviceWithPostponedRegistration : m_devicesWithPostponedRegistration)
        {
            handleSubdeviceRegistrationRequest(deviceWithPostponedRegistration.first,
                                               *deviceWithPostponedRegistration.second);
        }

        m_devicesWithPostponedRegistration.clear();
    }
}

void SubdeviceRegistrationService::handleSubdeviceRegistrationRequest(const std::string& deviceKey,
                                                                      const SubdeviceRegistrationRequest& request)
{
    LOG(TRACE) << METHOD_INFO;

    if (deviceKey == m_gatewayKey)
    {
        LOG(ERROR) << "SubdeviceRegistrationService: Skipping registration of gateway";
        return;
    }

    LOG(INFO) << "SubdeviceRegistrationService: Handling registration request for device with key '" << deviceKey
              << "'";

    auto savedDevice = m_deviceRepository.findByDeviceKey(deviceKey);
    auto subdeviceRequestingRegistration = std::unique_ptr<DetailedDevice>(
      new DetailedDevice(request.getSubdeviceName(), request.getSubdeviceKey(), request.getTemplate()));
    if (savedDevice && *savedDevice == *subdeviceRequestingRegistration)
    {
        LOG(WARN) << "SubdeviceRegistrationService: Ignoring device registration request for device with key '"
                  << deviceKey << "'. Already registered with given device info and device template";
        return;
    }

    std::lock_guard<decltype(m_devicesAwaitingRegistrationResponseMutex)> l{m_devicesAwaitingRegistrationResponseMutex};
    auto device = std::unique_ptr<DetailedDevice>(
      new DetailedDevice(request.getSubdeviceName(), request.getSubdeviceKey(), request.getTemplate()));
    m_devicesAwaitingRegistrationResponse[deviceKey] = std::move(device);

    std::shared_ptr<Message> registrationRequest = m_protocol.makeMessage(m_gatewayKey, request);
    if (!registrationRequest)
    {
        LOG(WARN) << "SubdeviceRegistrationService: Unable to create registration request message";
        return;
    }

    auto responseChannel = m_protocol.getResponseChannel(*registrationRequest, m_gatewayKey, deviceKey);
    RetryMessageStruct retryMessage{registrationRequest, responseChannel,
                                    [=](std::shared_ptr<Message>) {
                                        LOG(ERROR) << "Failed to register device with key: " << deviceKey
                                                   << ", no response from platform";
                                    },
                                    RETRY_COUNT, RETRY_TIMEOUT};
    m_platformRetryMessageHandler.addMessage(retryMessage);
}

void SubdeviceRegistrationService::handleSubdeviceRegistrationResponse(const std::string& deviceKey,
                                                                       const SubdeviceRegistrationResponse& response)
{
    LOG(TRACE) << METHOD_INFO;

    if (deviceKey == m_gatewayKey)
    {
        LOG(ERROR) << "SubdeviceRegistrationService: Ignoring registration response for gateway";
        return;
    }

    std::lock_guard<decltype(m_devicesAwaitingRegistrationResponseMutex)> l(m_devicesAwaitingRegistrationResponseMutex);
    if (m_devicesAwaitingRegistrationResponse.find(deviceKey) == m_devicesAwaitingRegistrationResponse.end())
    {
        LOG(ERROR)
          << "SubdeviceRegistrationService: Ignoring unexpected device registration response for device with key '"
          << deviceKey << "'";
        return;
    }

    const auto registrationResult = response.getResult();
    if (registrationResult == SubdeviceRegistrationResponse::Result::OK)
    {
        LOG(INFO) << "SubdeviceRegistrationService: Device with key '" << deviceKey
                  << "' successfully registered on platform";

        const auto& device = *m_devicesAwaitingRegistrationResponse.at(deviceKey);
        LOG(DEBUG) << "SubdeviceRegistrationService: Saving device with key '" << device.getKey()
                   << "' to device repository";

        m_deviceRepository.save(device);
        invokeOnDeviceRegisteredListener(deviceKey);
    }
    else
    {
        const auto registrationFailureReason = [&]() -> std::string {
            if (registrationResult == SubdeviceRegistrationResponse::Result::ERROR_KEY_CONFLICT)
            {
                return "Device with given key already registered";
            }
            else if (registrationResult ==
                     SubdeviceRegistrationResponse::Result::ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED)
            {
                return "Maximum number of devices registered";
            }
            else if (registrationResult == SubdeviceRegistrationResponse::Result::ERROR_INVALID_DTO)
            {
                return "Rejected registration DTO";
            }
            else if (registrationResult == SubdeviceRegistrationResponse::Result::ERROR_NOT_A_GATEWAY)
            {
                return "Device is not a gateway";
            }
            else if (registrationResult == SubdeviceRegistrationResponse::Result::ERROR_VALIDATION_ERROR)
            {
                return "Faulty registration request";
            }
            else if (registrationResult == SubdeviceRegistrationResponse::Result::ERROR_KEY_MISSING)
            {
                return "Key missing from registration request";
            }
            else if (registrationResult == SubdeviceRegistrationResponse::Result::ERROR_SUBDEVICE_MANAGEMENT_FORBIDDEN)
            {
                return "Subdevice management is forbidden for this gateway";
            }
            else if (registrationResult == SubdeviceRegistrationResponse::Result::ERROR_GATEWAY_NOT_FOUND)
            {
                return "Gateway has been deleted on platform";
            }
            else if (registrationResult == SubdeviceRegistrationResponse::Result::ERROR_UNKNOWN)
            {
                return "Unknown subdevice registration error";
            }

            assert(false && "Unknown subdevice registration error");
            return "Unknown";
        }();

        LOG(ERROR) << "SubdeviceRegistrationService: Unable to register device with key '" << deviceKey
                   << "'. Reason: '" << registrationFailureReason << "' Description: " << response.getDescription();
    }

    m_devicesAwaitingRegistrationResponse.erase(deviceKey);

    // send response to device
    std::shared_ptr<Message> registrationResponseMessage = m_protocol.makeMessage(deviceKey, response);
    if (!registrationResponseMessage)
    {
        LOG(WARN) << "SubdeviceRegistrationService: Unable to create registration response message";
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(registrationResponseMessage);
}

void SubdeviceRegistrationService::addToPostponedSubdeviceRegistrationRequests(
  const std::string& deviceKey, const wolkabout::SubdeviceRegistrationRequest& request)
{
    LOG(TRACE) << METHOD_INFO;

    LOG(INFO) << "SubdeviceRegistrationService: Postponing registration of device with key '" << deviceKey
              << "'. Waiting for gateway to be updated";

    std::lock_guard<decltype(m_devicesWithPostponedRegistrationMutex)> l(m_devicesWithPostponedRegistrationMutex);
    auto postponedDeviceRegistration =
      std::unique_ptr<SubdeviceRegistrationRequest>(new SubdeviceRegistrationRequest(request));
    m_devicesWithPostponedRegistration[deviceKey] = std::move(postponedDeviceRegistration);
}
}    // namespace wolkabout
