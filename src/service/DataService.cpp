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
#include "InboundMessageHandler.h"
#include "OutboundMessageHandler.h"
#include "model/ActuatorGetCommand.h"
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
                         OutboundMessageHandler& outboundDeviceMessageHandler, MessageListener* gatewayDevice)
: m_gatewayKey{gatewayKey}
, m_protocol{protocol}
, m_deviceRepository{deviceRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_gatewayDevice{gatewayDevice}
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
        routePlatformToGatewayMessage(message);
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

    routeDeviceToPlatformMessage(message);
}

const GatewayProtocol& DataService::getProtocol() const
{
    return m_protocol;
}

void DataService::addMessage(std::shared_ptr<Message> message)
{
    routeGatewayToPlatformMessage(message);
}

void DataService::setGatewayMessageListener(MessageListener* gatewayDevice)
{
    m_gatewayDevice = gatewayDevice;
}

void DataService::requestActuatorStatusesForDevice(const std::string& deviceKey)
{
    auto device = m_deviceRepository.findByDeviceKey(deviceKey);

    if (!device)
    {
        LOG(ERROR) << "DeviceStatusService::requestActuatorStatusesForDevice Device not found in repository: "
                   << deviceKey;
        return;
    }

    for (const auto& reference : device->getActuatorReferences())
    {
        std::shared_ptr<Message> message = m_protocol.makeMessage(deviceKey, ActuatorGetCommand(reference));
        m_outboundDeviceMessageHandler.addMessage(message);
    }
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

    if (m_gatewayDevice)
    {
        m_gatewayDevice->messageReceived(routedMessage);
    }
}
}    // namespace wolkabout
