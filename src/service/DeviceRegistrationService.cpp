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

#include "DeviceRegistrationService.h"
#include "DeviceManager.h"
#include "connectivity/json/RegistrationProtocol.h"
#include "model/DeviceRegistrationRequestDto.h"
#include "model/DeviceRegistrationResponseDto.h"
#include "model/Message.h"
#include "utilities/Logger.h"

namespace wolkabout
{
DeviceRegistrationService::DeviceRegistrationService(
  std::string gatewayKey, DeviceManager& deviceManager,
  std::shared_ptr<OutboundMessageHandler> outboundPlatformMessageHandler,
  std::shared_ptr<OutboundMessageHandler> outboundDeviceMessageHandler)
: m_gatewayKey{std::move(gatewayKey)}
, m_deviceManager{deviceManager}
, m_outboundPlatformMessageHandler{std::move(outboundPlatformMessageHandler)}
, m_outboundDeviceMessageHandler{std::move(outboundDeviceMessageHandler)}
{
}

void DeviceRegistrationService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << "Registration service: Platfom message received: " << message->getTopic() << " , "
               << message->getContent();

    if (RegistrationProtocol::getInstance().isPlatformToDeviceMessage(message->getTopic(), m_gatewayKey))
    {
        if (RegistrationProtocol::getInstance().isRegistrationResponse(message))
        {
            auto response = RegistrationProtocol::getInstance().makeRegistrationResponse(message);

            if (!response)
            {
                LOG(WARN) << "DeviceRegistrationResponse could not be parsed: " << message->getTopic() << " , "
                          << message->getContent();
                return;
            }

            handleDeviceRegistrationResponse(response);
        }
        else
        {
            LOG(WARN) << "Incorrect registration channel from platform: " << message->getTopic();
        }
    }
    else if (RegistrationProtocol::getInstance().isPlatformToGatewayMessage(message->getTopic(), m_gatewayKey))
    {
        if (RegistrationProtocol::getInstance().isRegistrationResponse(message))
        {
            auto response = RegistrationProtocol::getInstance().makeRegistrationResponse(message);

            if (!response)
            {
                LOG(WARN) << "DeviceRegistrationResponse could not be parsed: " << message->getTopic() << " , "
                          << message->getContent();
                return;
            }

            handleGatewayRegistrationResponse(response);
        }
        else
        {
            LOG(WARN) << "Incorrect registration channel from platform: " << message->getTopic();
        }
    }
    else
    {
        LOG(WARN) << "Message channel not parsed: " << message->getTopic();
    }
}

void DeviceRegistrationService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(DEBUG) << "Registration service: Device message received: " << message->getTopic() << " , "
               << message->getContent();

    if (RegistrationProtocol::getInstance().isDeviceToPlatformMessage(message->getTopic()))
    {
        if (RegistrationProtocol::getInstance().isRegistrationRequest(message))
        {
            auto request = RegistrationProtocol::getInstance().makeRegistrationRequest(message);

            if (!request)
            {
                LOG(WARN) << "DeviceRegistrationRequest could not be parsed: " << message->getTopic() << " , "
                          << message->getContent();
                return;
            }

            handleDeviceRegistrationRequest(request);
        }
        else
        {
            LOG(WARN) << "Incorrect registration channel from device: " << message->getTopic();
        }
    }
    else if (RegistrationProtocol::getInstance().isGatewayToPlatformMessage(message->getTopic(), m_gatewayKey))
    {
        if (RegistrationProtocol::getInstance().isRegistrationRequest(message))
        {
            auto request = RegistrationProtocol::getInstance().makeRegistrationRequest(message);

            if (!request)
            {
                LOG(WARN) << "DeviceRegistrationRequest could not be parsed: " << message->getTopic() << " , "
                          << message->getContent();
                return;
            }

            handleGatewayRegistrationRequest(request);
        }
        else
        {
            LOG(WARN) << "Incorrect registration channel from gateway: " << message->getTopic();
        }
    }
    else
    {
        LOG(WARN) << "Message channel not parsed: " << message->getTopic();
    }
}

void DeviceRegistrationService::handleDeviceRegistrationRequest(std::shared_ptr<DeviceRegistrationRequestDto> request)
{
    //	if(m_deviceRepository.deviceExists(request->getDeviceKey()))
    //	{

    //	}
    //	else
    //	{
    //		m_deviceRepository.save(std::make_shared<Device>(request->getDeviceKey(), request->getDeviceName(),
    //														 request->getManifest()));
    //	}
}

void DeviceRegistrationService::handleGatewayRegistrationRequest(std::shared_ptr<DeviceRegistrationRequestDto> request)
{
}

void DeviceRegistrationService::handleDeviceRegistrationResponse(
  std::shared_ptr<DeviceRegistrationResponseDto> response)
{
}

void DeviceRegistrationService::handleGatewayRegistrationResponse(
  std::shared_ptr<DeviceRegistrationResponseDto> response)
{
}
}
