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

#ifndef DATASERVICE_H
#define DATASERVICE_H

#include "DataServiceBase.h"
#include "OutboundMessageHandler.h"
#include "model/ActuatorStatus.h"
#include "model/Device.h"
#include "model/DeviceManifest.h"
#include "model/Message.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"

#include <algorithm>
#include <atomic>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
template <class P> class DataService : public DataServiceBase
{
public:
    DataService(const std::string& gatewayKey, DeviceRepository& deviceRepository,
                OutboundMessageHandler& outboundPlatformMessageHandler,
                OutboundMessageHandler& outboundDeviceMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    void connected() override;

    void disconnected() override;

private:
    void routeDeviceToPlatformMessage(std::shared_ptr<Message> message);
    void routePlatformToDeviceMessage(std::shared_ptr<Message> message);

    void routeGatewayToPlatformMessage(std::shared_ptr<Message> message);
    void routePlatformToGatewayMessage(std::shared_ptr<Message> message);

    void handleGatewayOfflineMessage(std::shared_ptr<Message> message);

    const std::string m_gatewayKey;
    DeviceRepository& m_deviceRepository;

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    OutboundMessageHandler& m_outboundDeviceMessageHandler;

    std::atomic_bool m_gatewayModuleConnected;
};

template <class P>
DataService<P>::DataService(const std::string& gatewayKey, DeviceRepository& deviceRepository,
                            OutboundMessageHandler& outboundPlatformMessageHandler,
                            OutboundMessageHandler& outboundDeviceMessageHandler)
: m_gatewayKey{gatewayKey}
, m_deviceRepository{deviceRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_gatewayModuleConnected{false}
{
}

template <class P> void DataService<P>::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = message->getChannel();

    if (!P::isMessageFromPlatform(topic))
    {
        LOG(WARN) << "DataService: Ignoring message on channel '" << topic << "'. Message not from platform.";
        return;
    }

    const std::string deviceKey = P::extractDeviceKeyFromChannel(topic);

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

template <class P> void DataService<P>::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = message->getChannel();
    if (!P::isMessageToPlatform(channel))
    {
        LOG(WARN) << "DataService: Ignoring message on channel '" << channel << "'. Message not intended for platform.";
        return;
    }

    const std::string deviceKey = P::extractDeviceKeyFromChannel(channel);
    const std::unique_ptr<Device> device = m_deviceRepository.findByDeviceKey(deviceKey);
    if (!device)
    {
        LOG(WARN) << "DataService: Not forwarding data message from device with key '" << deviceKey
                  << "'. Device not registered";
        return;
    }

    if (P::isSensorReadingMessage(channel))
    {
        const std::string sensorReference = P::extractReferenceFromChannel(channel);
        const DeviceManifest& deviceManifest = device->getManifest();
        if (!deviceManifest.hasSensorManifestWithReference(sensorReference))
        {
            LOG(WARN) << "DataService: Not forwarding sensor reading with reference '" << sensorReference
                      << "' from device with key '" << deviceKey
                      << "'. No sensor with given reference in device manifest";
            return;
        }
    }
    else if (P::isAlarmMessage(channel))
    {
        const std::string alarmReference = P::extractReferenceFromChannel(channel);
        const DeviceManifest& deviceManifest = device->getManifest();
        if (!deviceManifest.hasAlarmManifestWithReference(alarmReference))
        {
            LOG(WARN) << "DataService: Not forwarding alarm with reference '" << alarmReference
                      << "' from device with key '" << deviceKey
                      << "'. No event with given reference in device manifest";
            return;
        }
    }
    else if (P::isActuatorStatusMessage(channel))
    {
        const std::string actuatorReference = P::extractReferenceFromChannel(channel);
        const DeviceManifest& deviceManifest = device->getManifest();
        if (!deviceManifest.hasActuatorManifestWithReference(actuatorReference))
        {
            LOG(WARN) << "DataService: Not forwarding actuator status with reference '" << actuatorReference
                      << "' from device with key '" << deviceKey
                      << "'. No actuator with given reference in device manifest";
            return;
        }
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

template <class P> void DataService<P>::connected()
{
    m_gatewayModuleConnected = true;
}

template <class P> void DataService<P>::disconnected()
{
    m_gatewayModuleConnected = false;
}

template <class P> void DataService<P>::routeDeviceToPlatformMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = P::routeDeviceToPlatformMessage(message->getChannel(), m_gatewayKey);
    if (channel.empty())
    {
        LOG(WARN) << "Failed to route device message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), channel)};
    m_outboundPlatformMessageHandler.addMessage(routedMessage);
}

template <class P> void DataService<P>::routePlatformToDeviceMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = P::routePlatformToDeviceMessage(message->getChannel(), m_gatewayKey);
    if (channel.empty())
    {
        LOG(WARN) << "Failed to route platform message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), channel)};
    m_outboundDeviceMessageHandler.addMessage(routedMessage);
}

template <class P> void DataService<P>::routeGatewayToPlatformMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = P::routeGatewayToPlatformMessage(message->getChannel());
    if (channel.empty())
    {
        LOG(WARN) << "Failed to route device message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), channel)};
    m_outboundPlatformMessageHandler.addMessage(routedMessage);
}

template <class P> void DataService<P>::routePlatformToGatewayMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = P::routePlatformToGatewayMessage(message->getChannel());
    if (channel.empty())
    {
        LOG(WARN) << "Failed to route platform message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), channel)};
    m_outboundDeviceMessageHandler.addMessage(routedMessage);
}

template <class P> void DataService<P>::handleGatewayOfflineMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string ref = P::extractReferenceFromChannel(message->getChannel());
    if (ref.empty())
    {
        LOG(INFO) << "Data Service: Unable to get reference from topic: " << message->getChannel();
        return;
    }

    const auto gatewayDevice = m_deviceRepository.findByDeviceKey(m_gatewayKey);
    if (!gatewayDevice)
    {
        LOG(WARN) << "Data Service: Gateway device not found in repository";
        return;
    }

    const auto actuatorReferences = gatewayDevice->getActuatorReferences();
    if (auto it = std::find(actuatorReferences.begin(), actuatorReferences.end(), ref) != actuatorReferences.end())
    {
        ActuatorStatus status{"", ref, ActuatorStatus::State::ERROR};
        auto statusMessage = P::make(m_gatewayKey, status);

        if (!statusMessage)
        {
            LOG(WARN) << "Failed to create actuator status message";
            return;
        }

        m_outboundPlatformMessageHandler.addMessage(statusMessage);
    }
    // TODO configuration
    else
    {
        LOG(INFO) << "Data Service: Reference not defined for gateway: " << ref;
    }
}
}    // namespace wolkabout

#endif
