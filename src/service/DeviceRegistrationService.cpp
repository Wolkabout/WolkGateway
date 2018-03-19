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

#include "service/DeviceRegistrationService.h"
#include "OutboundMessageHandler.h"
#include "connectivity/json/DeviceRegistrationProtocol.h"
#include "model/Device.h"
#include "model/DeviceRegistrationRequest.h"
#include "model/DeviceRegistrationResponse.h"
#include "model/DeviceReregistrationResponse.h"
#include "model/Message.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"

#include <cassert>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace wolkabout
{
DeviceRegistrationService::DeviceRegistrationService(std::string gatewayKey, DeviceRepository& deviceRepository,
                                                     OutboundMessageHandler& outboundPlatformMessageHandler,
                                                     OutboundMessageHandler& outboundDeviceMessageHandler)
: m_gatewayKey{std::move(gatewayKey)}
, m_deviceRepository{deviceRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
{
}

void DeviceRegistrationService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    if (!DeviceRegistrationProtocol::isMessageFromPlatform(message->getChannel()))
    {
        LOG(WARN) << "DeviceRegistrationService: Ignoring message on channel '" << message->getChannel()
                  << "'. Message not from platform.";
        return;
    }

    if (DeviceRegistrationProtocol::isRegistrationResponse(message))
    {
        const auto response = DeviceRegistrationProtocol::makeRegistrationResponse(message);
        if (!response)
        {
            LOG(ERROR)
              << "DeviceRegistrationService: Device registration response could not be deserialized. Channel: '"
              << message->getChannel() << "' Payload: '" << message->getContent() << "'";
            return;
        }

        auto deviceKey = DeviceRegistrationProtocol::extractDeviceKeyFromChannel(message->getChannel());
        handleDeviceRegistrationResponse(deviceKey, *response);
    }
    else if (DeviceRegistrationProtocol::isReregistrationRequest(message))
    {
        handleDeviceReregistrationRequest();
    }
    else if (DeviceRegistrationProtocol::isDeviceDeletionResponse(message))
    {
        LOG(INFO) << "DeviceRegistrationService: Received device deletion response (" << message->getChannel() << ")";
    }
    else
    {
        LOG(WARN) << "DeviceRegistrationService: Unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
    }
}

void DeviceRegistrationService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    if (!DeviceRegistrationProtocol::isMessageToPlatform(message->getChannel()))
    {
        LOG(WARN) << "DeviceRegistrationService: Ignoring message received on channel '" << message->getChannel()
                  << "'. Message not intended for platform.";
        return;
    }

    if (!DeviceRegistrationProtocol::isRegistrationRequest(message))
    {
        LOG(WARN) << "DeviceRegistrationService: unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
        return;
    }

    auto request = DeviceRegistrationProtocol::makeRegistrationRequest(message);
    if (!request)
    {
        LOG(ERROR) << "DeviceRegistrationService: Device registration request could not be deserialized. Channel: '"
                   << message->getChannel() << "' Payload: '" << message->getContent() << "'";
        return;
    }

    auto deviceKey = DeviceRegistrationProtocol::extractDeviceKeyFromChannel(message->getChannel());
    if (!m_deviceRepository.containsDeviceWithKey(m_gatewayKey) && deviceKey != m_gatewayKey)
    {
        addToPostponedDeviceRegistartionRequests(deviceKey, *request);
        return;
    }

    handleDeviceRegistrationRequest(deviceKey, *request);
}

void DeviceRegistrationService::onDeviceRegistered(
  std::function<void(const std::string& deviceKey, bool isGateway)> onDeviceRegistered)
{
    m_onDeviceRegistered = onDeviceRegistered;
}

void DeviceRegistrationService::invokeOnDeviceRegisteredListener(const std::string& deviceKey, bool isGateway) const
{
    if (m_onDeviceRegistered)
    {
        m_onDeviceRegistered(deviceKey, isGateway);
    }
}

void DeviceRegistrationService::deleteDevicesOtherThan(const std::vector<std::string>& devicesKeys)
{
    const auto deviceKeysFromRepository = m_deviceRepository.findAllDeviceKeys();
    for (const std::string& deviceKeyFromRepository : *deviceKeysFromRepository)
    {
        if (std::find(devicesKeys.begin(), devicesKeys.end(), deviceKeyFromRepository) == devicesKeys.end())
        {
            if (deviceKeyFromRepository != m_gatewayKey)
            {
                m_deviceRepository.remove(deviceKeyFromRepository);
            }
            else
            {
                m_deviceRepository.removeAll();
            }

            const auto deviceDeletionRequestMessage =
              DeviceRegistrationProtocol::makeDeviceDeletionRequestMessage(m_gatewayKey, deviceKeyFromRepository);
            m_outboundPlatformMessageHandler.addMessage(deviceDeletionRequestMessage);
        }
    }
}

void DeviceRegistrationService::handleDeviceRegistrationRequest(const std::string& deviceKey,
                                                                const DeviceRegistrationRequest& request)
{
    LOG(TRACE) << METHOD_INFO;

    LOG(INFO) << "DeviceRegistrationService: Handling registration request for device with key '" << deviceKey << "'";

    auto gateway = m_deviceRepository.findByDeviceKey(m_gatewayKey);
    if (gateway && gateway->getManifest().getProtocol() != request.getManifest().getProtocol())
    {
        LOG(ERROR) << "DeviceRegistrationService: Ignoring device registration request for device with key '"
                   << deviceKey << "'. Gateway uses protocol '" << gateway->getManifest().getProtocol()
                   << "' but device wants to register with protocol '" << request.getManifest().getProtocol() << "'";
        return;
    }

    auto savedDevice = m_deviceRepository.findByDeviceKey(deviceKey);
    auto deviceRequestingRegistration =
      std::unique_ptr<Device>(new Device(request.getDeviceName(), request.getDeviceKey(), request.getManifest()));
    if (savedDevice && *savedDevice == *deviceRequestingRegistration)
    {
        LOG(WARN) << "DeviceRegistrationService: Ignoring device registration request for device with key '"
                  << deviceKey << "'. Already registered with given device info and device manifest";
        return;
    }

    std::lock_guard<decltype(m_devicesAwaitingRegistrationResponseMutex)> l(m_devicesAwaitingRegistrationResponseMutex);
    auto device =
      std::unique_ptr<Device>(new Device(request.getDeviceName(), request.getDeviceKey(), request.getManifest()));
    m_devicesAwaitingRegistrationResponse[deviceKey] = std::move(device);

    const auto registrationRequest =
      DeviceRegistrationProtocol::makeDeviceRegistrationRequestMessage(m_gatewayKey, deviceKey, request);
    m_outboundPlatformMessageHandler.addMessage(registrationRequest);
}

void DeviceRegistrationService::handleDeviceReregistrationRequest()
{
    LOG(TRACE) << METHOD_INFO;

    LOG(INFO) << "DeviceRegistrationService: Reregistering devices connected to gateway";

    const auto reregistrationResponse = DeviceReregistrationResponse(DeviceReregistrationResponse::Result::OK);
    const auto reregistrationResponseMessage =
      DeviceRegistrationProtocol::makeDeviceReregistrationResponseMessage(m_gatewayKey, reregistrationResponse);
    m_outboundPlatformMessageHandler.addMessage(reregistrationResponseMessage);

    const auto registeredDevicesKeys = m_deviceRepository.findAllDeviceKeys();
    for (const std::string& deviceKey : *registeredDevicesKeys)
    {
        m_deviceRepository.remove(deviceKey);
    }

    const auto deviceRegistrationRequest = DeviceRegistrationProtocol::makeDeviceReregistrationRequestForDevice();
    m_outboundDeviceMessageHandler.addMessage(deviceRegistrationRequest);
}

void DeviceRegistrationService::handleDeviceRegistrationResponse(const std::string& deviceKey,
                                                                 const DeviceRegistrationResponse& response)
{
    LOG(TRACE) << METHOD_INFO;

    std::lock_guard<decltype(m_devicesAwaitingRegistrationResponseMutex)> l(m_devicesAwaitingRegistrationResponseMutex);
    if (m_devicesAwaitingRegistrationResponse.find(deviceKey) == m_devicesAwaitingRegistrationResponse.end())
    {
        LOG(ERROR)
          << "DeviceRegistrationService: Ignoring unexpected device registration response for device with key '"
          << deviceKey << "'";
        return;
    }

    const auto registrationResult = response.getResult();
    if (registrationResult == DeviceRegistrationResponse::Result::OK)
    {
        LOG(INFO) << "DeviceRegistrationService: Device with key '" << deviceKey
                  << "' successfully registered on platform";

        const auto& device = *m_devicesAwaitingRegistrationResponse.at(deviceKey);
        LOG(DEBUG) << "DeviceRegistrationService: Saving device with key '" << device.getKey()
                   << "' to device repository";

        m_deviceRepository.save(device);
        invokeOnDeviceRegisteredListener(deviceKey, deviceKey == m_gatewayKey);

        std::lock_guard<decltype(m_devicesWithPostponedRegistrationMutex)> devicesWithPostponedRegistrationLock(
          m_devicesWithPostponedRegistrationMutex);
        if (device.getKey() == m_gatewayKey && !m_devicesWithPostponedRegistration.empty())
        {
            LOG(INFO) << "DeviceRegistrationService: Processing postponed device registration requests";

            for (const auto& deviceWithPostponedRegistration : m_devicesWithPostponedRegistration)
            {
                handleDeviceRegistrationRequest(deviceWithPostponedRegistration.first,
                                                *deviceWithPostponedRegistration.second);
            }

            m_devicesWithPostponedRegistration.clear();
        }
    }
    else
    {
        const auto registrationFailureReason = [&]() -> std::string {
            if (registrationResult == DeviceRegistrationResponse::Result::ERROR_KEY_CONFLICT)
            {
                return "Device with given key already registered";
            }
            else if (registrationResult == DeviceRegistrationResponse::Result::ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED)
            {
                return "Maximum number of devices registered";
            }
            else if (registrationResult == DeviceRegistrationResponse::Result::ERROR_READING_PAYLOAD)
            {
                return "Rejected registration DTO";
            }
            else if (registrationResult == DeviceRegistrationResponse::Result::ERROR_MANIFEST_CONFLICT)
            {
                return "Manifest conflict";
            }
            else if (registrationResult == DeviceRegistrationResponse::Result::ERROR_NO_GATEWAY_MANIFEST)
            {
                return "Gateway has been deleted on platform";
            }
            else if (registrationResult == DeviceRegistrationResponse::Result::ERROR_GATEWAY_NOT_FOUND)
            {
                return "Gateway has been deleted on platform";
            }

            assert(false && "Unknown device registration error");
            return "Unknown";
        }();

        LOG(ERROR) << "DeviceRegistrationService: Unable to register device with key '" << deviceKey
                   << "'. Reason: " << registrationFailureReason;
    }

    m_devicesAwaitingRegistrationResponse.erase(deviceKey);
}

void DeviceRegistrationService::addToPostponedDeviceRegistartionRequests(
  const std::string& deviceKey, const wolkabout::DeviceRegistrationRequest& request)
{
    LOG(TRACE) << METHOD_INFO;

    LOG(INFO) << "DeviceRegistrationService: Postponing registration of device with key '" << deviceKey
              << "'. Waiting for gateway to be registered";

    std::lock_guard<decltype(m_devicesWithPostponedRegistrationMutex)> l(m_devicesWithPostponedRegistrationMutex);
    auto postponedDeviceRegistration =
      std::unique_ptr<DeviceRegistrationRequest>(new DeviceRegistrationRequest(request));
    m_devicesWithPostponedRegistration[deviceKey] = std::move(postponedDeviceRegistration);
}
}    // namespace wolkabout
