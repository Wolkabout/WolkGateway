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

#include "service/DataService.h"
#include "OutboundMessageHandler.h"
#include "model/ActuatorStatus.h"
#include "model/DetailedDevice.h"
#include "model/Message.h"
#include "protocol/GatewayDataProtocol.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"

#include <algorithm>
#include <cassert>

namespace wolkabout
{
DataService::DataService(const std::string& gatewayKey, GatewayDataProtocol& protocol,
                         DeviceRepository& deviceRepository, OutboundMessageHandler& outboundPlatformMessageHandler,
                         OutboundMessageHandler& outboundDeviceMessageHandler)
: m_gatewayKey{gatewayKey}
, m_protocol{protocol}
, m_deviceRepository{deviceRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_gatewayModuleConnected{false}
{
}

void DataService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = message->getChannel();

    if (!m_protocol.isMessageFromPlatform(*message))
    {
        LOG(WARN) << "DataService: Ignoring message on channel '" << topic << "'. Message not from platform.";
        return;
    }

    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(topic);

    if (deviceKey.empty())
    {
        LOG(WARN) << "DataService: Failed to extract device key from channel '" << topic << "'";
        return;
    }

    if (m_gatewayKey == deviceKey)
    {
        if (m_gatewayModuleConnected)
        {
            routePlatformToGatewayMessage(message);
        }
        else
        {
            handleGatewayOfflineMessage(message);
        }
    }
    else
    {
        // if message is for device remove gateway info from channel
        routePlatformToDeviceMessage(message);
    }
}

void DataService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = message->getChannel();
    if (!m_protocol.isMessageToPlatform(*message))
    {
        LOG(WARN) << "DataService: Ignoring message on channel '" << channel << "'. Message not intended for platform.";
        return;
    }

    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(channel);
    const std::unique_ptr<DetailedDevice> device = m_deviceRepository.findByDeviceKey(deviceKey);
    if (!device)
    {
        LOG(WARN) << "DataService: Not forwarding data message from device with key '" << deviceKey
                  << "'. Device not registered";
        return;
    }

    if (m_protocol.isSensorReadingMessage(*message))
    {
        const std::string sensorReference = m_protocol.extractReferenceFromChannel(channel);
        const DeviceManifest& deviceManifest = device->getManifest();
        if (!deviceManifest.hasSensorManifestWithReference(sensorReference))
        {
            LOG(WARN) << "DataService: Not forwarding sensor reading with reference '" << sensorReference
                      << "' from device with key '" << deviceKey
                      << "'. No sensor with given reference in device manifest";
            return;
        }
    }
    else if (m_protocol.isAlarmMessage(*message))
    {
        const std::string alarmReference = m_protocol.extractReferenceFromChannel(channel);
        const DeviceManifest& deviceManifest = device->getManifest();
        if (!deviceManifest.hasAlarmManifestWithReference(alarmReference))
        {
            LOG(WARN) << "DataService: Not forwarding alarm with reference '" << alarmReference
                      << "' from device with key '" << deviceKey
                      << "'. No event with given reference in device manifest";
            return;
        }
    }
    else if (m_protocol.isActuatorStatusMessage(*message))
    {
        const std::string actuatorReference = m_protocol.extractReferenceFromChannel(channel);
        const DeviceManifest& deviceManifest = device->getManifest();
        if (!deviceManifest.hasActuatorManifestWithReference(actuatorReference))
        {
            LOG(WARN) << "DataService: Not forwarding actuator status with reference '" << actuatorReference
                      << "' from device with key '" << deviceKey
                      << "'. No actuator with given reference in device manifest";
            return;
        }
    }
    else if (m_protocol.isConfigurationCurrentMessage(*message))
    {
    }
    else
    {
        assert(false && "DataService: Unsupported message type");

        LOG(ERROR) << "DataService: Not forwarding message from device on channel: '" << channel
                   << "'. Unsupported message type";
        return;
    }

    if (deviceKey == m_gatewayKey)
    {
        m_gatewayModuleConnected = true;
        routeGatewayToPlatformMessage(message);
    }
    else
    {
        routeDeviceToPlatformMessage(message);
    }
}

const GatewayProtocol& DataService::getProtocol() const
{
    return m_protocol;
}

void DataService::connected()
{
    m_gatewayModuleConnected = true;
}

void DataService::disconnected()
{
    m_gatewayModuleConnected = false;
}

void DataService::routeDeviceToPlatformMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = m_protocol.routeDeviceToPlatformMessage(message->getChannel(), m_gatewayKey);
    if (channel.empty())
    {
        LOG(WARN) << "Failed to route device message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), channel)};
    m_outboundPlatformMessageHandler.addMessage(routedMessage);
}

void DataService::routePlatformToDeviceMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = m_protocol.routePlatformToDeviceMessage(message->getChannel(), m_gatewayKey);
    if (channel.empty())
    {
        LOG(WARN) << "Failed to route platform message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), channel)};
    m_outboundDeviceMessageHandler.addMessage(routedMessage);
}

void DataService::routeGatewayToPlatformMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = m_protocol.routeGatewayToPlatformMessage(message->getChannel());
    if (channel.empty())
    {
        LOG(WARN) << "Failed to route device message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), channel)};
    m_outboundPlatformMessageHandler.addMessage(routedMessage);
}

void DataService::routePlatformToGatewayMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = m_protocol.routePlatformToGatewayMessage(message->getChannel());
    if (channel.empty())
    {
        LOG(WARN) << "Failed to route platform message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), channel)};
    m_outboundDeviceMessageHandler.addMessage(routedMessage);
}

void DataService::handleGatewayOfflineMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const auto gatewayDevice = m_deviceRepository.findByDeviceKey(m_gatewayKey);
    if (!gatewayDevice)
    {
        LOG(WARN) << "Data Service: Gateway device not found in repository";
        return;
    }

    if (m_protocol.isActuatorGetMessage(*message) || m_protocol.isActuatorSetMessage(*message))
    {
        LOG(DEBUG) << "Data Service: Handling actuation message for gateway";

        const std::string ref = m_protocol.extractReferenceFromChannel(message->getChannel());
        if (ref.empty())
        {
            LOG(WARN) << "Data Service: Unable to get reference from topic: " << message->getChannel();
            return;
        }

        const auto actuatorReferences = gatewayDevice->getActuatorReferences();
        if (auto it = std::find(actuatorReferences.begin(), actuatorReferences.end(), ref) != actuatorReferences.end())
        {
            ActuatorStatus status{"", ref, ActuatorStatus::State::ERROR};
            std::shared_ptr<Message> statusMessage = m_protocol.makeMessage(m_gatewayKey, status);

            if (!statusMessage)
            {
                LOG(WARN) << "Failed to create actuator status message";
                return;
            }

            m_outboundPlatformMessageHandler.addMessage(statusMessage);
        }
        else
        {
            LOG(INFO) << "Data Service: Actuator reference not defined for gateway: " << ref;
        }
    }
    else if (m_protocol.isConfigurationGetMessage(*message) || m_protocol.isConfigurationSetMessage(*message))
    {
        LOG(DEBUG) << "Data Service: Handling configuration message for gateway";
    }
    else
    {
        assert(false && "DataService: Unsupported message type");

        LOG(ERROR) << "DataService: Not forwarding message from device on channel: '" << message->getChannel()
                   << "'. Unsupported message type";
    }
}
}
