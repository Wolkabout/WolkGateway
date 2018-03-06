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
#include "Poco/Bugcheck.h"
#include "connectivity/json/DeviceRegistrationProtocol.h"
#include "model/Device.h"
#include "model/DeviceRegistrationRequestDto.h"
#include "model/DeviceRegistrationResponseDto.h"
#include "model/DeviceReregistrationResponseDto.h"
#include "model/Message.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace wolkabout
{
DeviceRegistrationService::DeviceRegistrationService(std::string gatewayKey, DeviceRepository& deviceRepository,
                                                     OutboundMessageHandler& outboundPlatformMessageHandler)
: m_gatewayKey{std::move(gatewayKey)}
, m_deviceRepository{deviceRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
{
}

void DeviceRegistrationService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    LOG(DEBUG) << "DeviceRegistrationService: Platfom message received: " << message->getChannel() << " , "
               << message->getContent();

    if (!DeviceRegistrationProtocol::getInstance().isMessageFromPlatform(message->getChannel()))
    {
        LOG(WARN) << "DeviceRegistrationService: Ignoring message on channel '" << message->getChannel()
                  << "'. Message not from platform.";
        return;
    }

    if (DeviceRegistrationProtocol::getInstance().isRegistrationResponse(message))
    {
        auto response = DeviceRegistrationProtocol::getInstance().makeRegistrationResponse(message);
        if (!response)
        {
            LOG(ERROR)
              << "DeviceRegistrationService: Device registration response could not be deserialized. Channel: '"
              << message->getChannel() << "' Payload: '" << message->getContent() << "'";
            return;
        }

        auto deviceKey = DeviceRegistrationProtocol::getInstance().extractDeviceKeyFromChannel(message->getChannel());
        handleDeviceRegistrationResponse(deviceKey, *response);
    }
    else if (DeviceRegistrationProtocol::getInstance().isReregistrationRequest(message))
    {
        handleDeviceReregistrationRequest();
    }
    else
    {
        LOG(WARN) << "DeviceRegistrationService: Unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
    }
}

void DeviceRegistrationService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    LOG(DEBUG) << "DeviceRegistrationService: Device message received: " << message->getChannel() << " , "
               << message->getContent();

    if (!DeviceRegistrationProtocol::getInstance().isMessageToPlatform(message->getChannel()))
    {
        LOG(WARN) << "DeviceRegistrationService: Ignoring message received on channel '" << message->getChannel()
                  << "'. Message not intended for platform.";
        return;
    }

    if (!DeviceRegistrationProtocol::getInstance().isRegistrationRequest(message))
    {
        LOG(WARN) << "DeviceRegistrationService: unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
        return;
    }

    auto request = DeviceRegistrationProtocol::getInstance().makeRegistrationRequest(message);
    if (!request)
    {
        LOG(ERROR) << "DeviceRegistrationService: Device registration request could not be deserialized. Channel: '"
                   << message->getChannel() << "' Payload: '" << message->getContent() << "'";
        return;
    }

    auto deviceKey = DeviceRegistrationProtocol::getInstance().extractDeviceKeyFromChannel(message->getChannel());
    if (!m_deviceRepository.containsDeviceWithKey(m_gatewayKey))
    {
        if (deviceKey == m_gatewayKey)
        {
            handleDeviceRegistrationRequest(deviceKey, *request);
        }
        else
        {
            addToPostponedDeviceRegistartionRequests(deviceKey, *request);
        }

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

void DeviceRegistrationService::handleDeviceRegistrationRequest(const std::string& deviceKey,
                                                                const DeviceRegistrationRequestDto& request)
{
    LOG(DEBUG) << METHOD_INFO;

    LOG(INFO) << "DeviceRegistrationService: Handling registration request for device with key '" << deviceKey << "'";

    auto gateway = m_deviceRepository.findByDeviceKey(m_gatewayKey);
    if (gateway && gateway->getManifest().getProtocol() != request.getManifest().getProtocol())
    {
        LOG(DEBUG) << "DeviceRegistrationService: Ignoring device registration request for device with key '"
                   << deviceKey << "'. Gateway uses protocol '" << gateway->getManifest().getProtocol()
                   << "' but device wants to register with protocol '" << request.getManifest().getProtocol() << "'";
        return;
    }

    auto savedDevice = m_deviceRepository.findByDeviceKey(deviceKey);
    auto deviceRequestingRegistration =
      std::unique_ptr<Device>(new Device(request.getDeviceName(), request.getDeviceKey(), request.getManifest()));
    if (savedDevice && *savedDevice == *deviceRequestingRegistration)
    {
        LOG(DEBUG) << "DeviceRegistrationService: Ignoring device registration request for device with key '"
                   << deviceKey << "'. No change in device info and manifest";
        return;
    }

    std::lock_guard<decltype(m_devicesAwaitingRegistrationResponseMutex)> l(m_devicesAwaitingRegistrationResponseMutex);
    auto device =
      std::unique_ptr<Device>(new Device(request.getDeviceName(), request.getDeviceKey(), request.getManifest()));
    m_devicesAwaitingRegistrationResponse[deviceKey] = std::move(device);

    auto registrationRequest = DeviceRegistrationProtocol::getInstance().makeMessage(m_gatewayKey, deviceKey, request);
    m_outboundPlatformMessageHandler.addMessage(registrationRequest);
}

void DeviceRegistrationService::handleDeviceReregistrationRequest()
{
    LOG(DEBUG) << METHOD_INFO;

    LOG(INFO) << "DeviceRegistrationService: Reregistering devices connected to gateway";

    DeviceReregistrationResponseDto reregistrationResponse(DeviceReregistrationResponseDto::Result::OK);
    m_outboundPlatformMessageHandler.addMessage(
      DeviceRegistrationProtocol::getInstance().makeMessage(m_gatewayKey, reregistrationResponse));

    auto registeredDevicesKeys = m_deviceRepository.findAllDeviceKeys();
    for (const std::string& deviceKey : *registeredDevicesKeys)
    {
        LOG(INFO) << "DeviceRegistrationService: Reregistering device with key '" << deviceKey << "'";

        auto device = m_deviceRepository.findByDeviceKey(deviceKey);
        auto deviceRegistrationRequest =
          std::make_shared<DeviceRegistrationRequestDto>(device->getName(), device->getKey(), device->getManifest());

        std::lock_guard<decltype(m_devicesAwaitingRegistrationResponseMutex)> l(
          m_devicesAwaitingRegistrationResponseMutex);
        m_devicesAwaitingRegistrationResponse[deviceKey] = std::move(device);
        m_deviceRepository.remove(deviceKey);

        m_outboundPlatformMessageHandler.addMessage(
          DeviceRegistrationProtocol::getInstance().makeMessage(m_gatewayKey, deviceKey, *deviceRegistrationRequest));
    }
}

void DeviceRegistrationService::handleDeviceRegistrationResponse(const std::string& deviceKey,
                                                                 const DeviceRegistrationResponseDto& response)
{
    LOG(DEBUG) << METHOD_INFO;

    std::lock_guard<decltype(m_devicesAwaitingRegistrationResponseMutex)> l(m_devicesAwaitingRegistrationResponseMutex);
    if (m_devicesAwaitingRegistrationResponse.find(deviceKey) == m_devicesAwaitingRegistrationResponse.end())
    {
        LOG(ERROR)
          << "DeviceRegistrationService: Ignoring unexpected device registration response for device with key '"
          << deviceKey << "'";
        return;
    }

    auto registrationResult = response.getResult();
    if (registrationResult == DeviceRegistrationResponseDto::Result::OK)
    {
        LOG(INFO) << "DeviceRegistrationService: Device with key '" << deviceKey
                  << "' successfully registered on platform";

        const auto& device = *m_devicesAwaitingRegistrationResponse.at(deviceKey);
        LOG(DEBUG) << "DeviceRegistrationService: Saving device with key '" << device.getKey()
                   << "' to device repository";

        m_deviceRepository.save(device);
        invokeOnDeviceRegisteredListener(deviceKey, deviceKey == m_gatewayKey);

        if (device.getKey() == m_gatewayKey)
        {
            LOG(INFO) << "DeviceRegistrationService: Processing postponed device registration requests";

            std::lock_guard<decltype(m_devicesWithPostponedRegistrationMutex)> devicesWithPostponedRegistrationLock(
              m_devicesWithPostponedRegistrationMutex);
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
        auto registrationFailureReason = [&]() -> std::string {
            if (registrationResult == DeviceRegistrationResponseDto::Result::ERROR_KEY_CONFLICT)
            {
                return "Device with given key already registered";
            }
            else if (registrationResult ==
                     DeviceRegistrationResponseDto::Result::ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED)
            {
                return "Maximum number of devices registered";
            }
            else if (registrationResult == DeviceRegistrationResponseDto::Result::ERROR_READING_PAYLOAD)
            {
                return "Rejected registration DTO";
            }
            else if (registrationResult == DeviceRegistrationResponseDto::Result::ERROR_MANIFEST_CONFLICT)
            {
                return "Manifest conflict";
            }
            else if (registrationResult == DeviceRegistrationResponseDto::Result::ERROR_NO_GATEWAY_MANIFEST)
            {
                return "Gateway has been deleted on platform";
            }
            else if (registrationResult == DeviceRegistrationResponseDto::Result::ERROR_GATEWAY_NOT_FOUND)
            {
                return "Gateway has been deleted on platform";
            }

            poco_assert_dbg(false);
            return "Unknown";
        }();

        LOG(ERROR) << "DeviceRegistrationService: Unable to register device with key '" << deviceKey
                   << "'. Reason: " << registrationFailureReason;
    }

    m_devicesAwaitingRegistrationResponse.erase(deviceKey);
}

void DeviceRegistrationService::addToPostponedDeviceRegistartionRequests(
  const std::string& deviceKey, const wolkabout::DeviceRegistrationRequestDto& request)
{
    LOG(DEBUG) << METHOD_INFO;

    LOG(INFO) << "DeviceRegistrationService: Postponing registration of device with key '" << deviceKey
              << "'. Waiting for gateway to be registered";

    std::lock_guard<decltype(m_devicesWithPostponedRegistrationMutex)> l(m_devicesWithPostponedRegistrationMutex);
    auto postponedDeviceRegistration =
      std::unique_ptr<DeviceRegistrationRequestDto>(new DeviceRegistrationRequestDto(request));
    m_devicesWithPostponedRegistration[deviceKey] = std::move(postponedDeviceRegistration);
}
}    // namespace wolkabout
