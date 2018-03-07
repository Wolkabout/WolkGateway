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
    void routeDeviceMessage(std::shared_ptr<Message> message);
    void routePlatformMessage(std::shared_ptr<Message> message);

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

    if (P::isPlatformToGatewayMessage(message->getChannel()))
    {
        if (m_gatewayModuleConnected)
        {
            // if message is for gateway device just resend it
            m_outboundDeviceMessageHandler.addMessage(message);
        }
        else
        {
            handleGatewayOfflineMessage(message);
        }
    }
    else if (P::isPlatformToDeviceMessage(message->getChannel()))
    {
        // if message is for device remove gateway info from channel
        routePlatformMessage(message);
    }
    else
    {
        LOG(WARN) << "Message channel not parsed: " << message->getChannel();
    }
}

template <class P> void DataService<P>::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    if (P::isGatewayToPlatformMessage(message->getChannel()))
    {
        // if message is from gateway device just resend it
        m_outboundPlatformMessageHandler.addMessage(message);

        // gateway module is connected
        m_gatewayModuleConnected = true;
    }
    else if (P::isDeviceToPlatformMessage(message->getChannel()))
    {
        // if message is from device add gateway info to channel
        routeDeviceMessage(message);
    }
    else
    {
        LOG(WARN) << "Message channel not parsed: " << message->getChannel();
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

template <class P> void DataService<P>::routeDeviceMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = P::routeDeviceMessage(message->getChannel(), m_gatewayKey);
    if (topic.empty())
    {
        LOG(WARN) << "Failed to route device message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), topic)};

    m_outboundPlatformMessageHandler.addMessage(routedMessage);
}

template <class P> void DataService<P>::routePlatformMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string topic = P::routePlatformMessage(message->getChannel(), m_gatewayKey);
    if (topic.empty())
    {
        LOG(WARN) << "Failed to route platform message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), topic)};

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

    auto gwDevice = m_deviceRepository.findByDeviceKey(m_gatewayKey);
    if (!gwDevice)
    {
        LOG(WARN) << "Data Service: Gateway device not found in repository";
        return;
    }

    auto actuatorReferences = gwDevice->getActuatorReferences();
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
