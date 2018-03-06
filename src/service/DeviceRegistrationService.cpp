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
#include "connectivity/json/RegistrationProtocol.h"
#include "model/Device.h"
#include "model/DeviceRegistrationRequestDto.h"
#include "model/DeviceRegistrationResponseDto.h"
#include "model/DeviceReregistrationResponseDto.h"
#include "model/Message.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"

#include <map>
#include <memory>
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

    LOG(DEBUG) << "Device registration service: Platfom message received: " << message->getChannel() << " , "
               << message->getContent();

    if (RegistrationProtocol::isMessageFromPlatform(message->getChannel(), m_gatewayKey))
    {
        LOG(WARN) << "Device registration service: Ignoring message on channel '" << message->getChannel();
        return;
    }

    if (RegistrationProtocol::isRegistrationResponse(message))
    {
        auto response = RegistrationProtocol::makeRegistrationResponse(message);
        if (!response)
        {
            LOG(ERROR) << "Device registration service: Device registration response could not be parsed. "
                       << "Channel: '" << message->getChannel() << "' Content: '" << message->getContent() << "'";
            return;
        }

        auto deviceKey = RegistrationProtocol::getDeviceKeyFromChannel(message->getChannel());
        handleRegistrationResponse(deviceKey, *response);
    }
    else if (RegistrationProtocol::isReregistrationRequest(message))
    {
        LOG(INFO) << "Device registration service: Reregisteding devices";

        handleReregistrationRequest();
    }
    else
    {
        LOG(WARN) << "Device registration service: Unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
    }
}

void DeviceRegistrationService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << METHOD_INFO;

    LOG(DEBUG) << "Device registration service: Device message received: " << message->getChannel() << " , "
               << message->getContent();

    if (!RegistrationProtocol::isMessageToPlatform(message->getChannel(), m_gatewayKey))
    {
        LOG(WARN) << "Device registration service: Ignoring message received on channel '" << message->getChannel();
        return;
    }

    if (RegistrationProtocol::isRegistrationRequest(message))
    {
        auto request = RegistrationProtocol::makeRegistrationRequest(message);
        if (!request)
        {
            LOG(WARN) << "Device registration service: Device registration request could not be parsed: "
                      << message->getChannel() << " , " << message->getContent();
            return;
        }

        auto deviceKey = RegistrationProtocol::getDeviceKeyFromChannel(message->getChannel());
        if (!m_deviceRepository.containsDeviceWithKey(deviceKey))
        {
            LOG(INFO) << "Device registration service: Handling registration of new device with key '" << deviceKey
                      << "'";
            handleRegistrationRequest(deviceKey, *request);
            return;
        }

        auto savedDevice = m_deviceRepository.findByDeviceKey(deviceKey);
        auto deviceRequestingRegistration =
          std::make_shared<Device>(request->getDeviceName(), request->getDeviceKey(), request->getManifest());
        if (*savedDevice != *deviceRequestingRegistration)
        {
            LOG(INFO) << "Device registration service: Handling registration of existing device with key '" << deviceKey
                      << "' - Device/Manifest change";
            handleRegistrationRequest(deviceKey, *request);
        }
    }
    else
    {
        LOG(WARN) << "Device registration service: unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
    }
}

void DeviceRegistrationService::onGatewayRegistered(std::function<void()> callback)
{
    m_gatewayRegisteredCallback = callback;
}

void DeviceRegistrationService::handleRegistrationRequest(const std::string& deviceKey,
                                                          const DeviceRegistrationRequestDto& request)
{
    LOG(DEBUG) << METHOD_INFO;

    LOG(INFO) << "Device registration service: Handling registration request for device with key '" << deviceKey << "'";

    auto device =
      std::unique_ptr<Device>(new Device(request.getDeviceName(), request.getDeviceKey(), request.getManifest()));
    m_pendingRegistrationDevices[deviceKey] = std::move(device);

    auto registrationRequest = RegistrationProtocol::make(m_gatewayKey, deviceKey, request);
    m_outboundPlatformMessageHandler.addMessage(registrationRequest);
}

void DeviceRegistrationService::handleRegistrationResponse(const std::string& deviceKey,
                                                           const DeviceRegistrationResponseDto& response)
{
    LOG(DEBUG) << METHOD_INFO;

    if (m_pendingRegistrationDevices.find(deviceKey) == m_pendingRegistrationDevices.end())
    {
        LOG(ERROR)
          << "Device registration service: Ignoring unexpected device registration response for device with key '"
          << deviceKey << "'";
        return;
    }

    auto registrationResult = response.getResult();
    if (registrationResult == DeviceRegistrationResponseDto::Result::OK)
    {
        LOG(INFO) << "Device registration service: Device with key '" << deviceKey
                  << "' successfully registered on platform";

        const auto& device = *m_pendingRegistrationDevices.at(deviceKey);
        LOG(DEBUG) << "Device registration service: Saving device with key '" << device.getKey()
                   << "' to device repository";

        if (m_deviceRepository.containsDeviceWithKey(device.getKey()))
        {
            m_deviceRepository.update(device);
        }
        else
        {
            m_deviceRepository.save(device);
        }

        if (deviceKey == m_gatewayKey && m_gatewayRegisteredCallback)
        {
            m_gatewayRegisteredCallback();
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

        LOG(ERROR) << "Device registration service: Unable to register device with key '" << deviceKey
                   << "'. Reason: " << registrationFailureReason;
    }

    m_pendingRegistrationDevices.erase(deviceKey);
}

void DeviceRegistrationService::handleReregistrationRequest()
{
    LOG(DEBUG) << METHOD_INFO;

    // m_outboundPlatformMessageHandler.addMessage(std::make_shared<Message>("contenxt", "channel"));
}
}    // namespace wolkabout
