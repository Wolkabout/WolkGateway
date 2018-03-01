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

#ifndef WOLK_H
#define WOLK_H

#include "ChannelProtocolResolver.h"
#include "InboundDeviceMessageHandler.h"
#include "WolkBuilder.h"
#include "model/Device.h"
#include "repository/DeviceRepository.h"
#include "service/DataService.h"
#include "utilities/CommandBuffer.h"
#include "utilities/StringUtils.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <typeindex>
#include <vector>

namespace wolkabout
{
class ConnectivityService;
class InboundMessageHandler;
class InboundDeviceMessageHandler;
class InboundPlatformMessageHandler;
// class FirmwareUpdateService;
// class FileDownloadService;
class DeviceManager;
class PublishingService;
class OutboundServiceDataHandler;
class DataServiceBase;
class DeviceRegistrationService;

class Wolk
{
    friend class WolkBuilder;

public:
    virtual ~Wolk() = default;

    /**
     * @brief Initiates wolkabout::WolkBuilder that configures device to connect to WolkAbout IoT Cloud
     * @param device wolkabout::Device
     * @return wolkabout::WolkBuilder instance
     */
    static WolkBuilder newBuilder(Device device);

    /**
     * @brief connect Establishes connection with WolkAbout IoT platform
     */
    void connect();

    /**
     * @brief disconnect Disconnects from WolkAbout IoT platform
     */
    void disconnect();

private:
    class ConnectivityFacade;

    Wolk(Device device);

    void addToCommandBuffer(std::function<void()> command);

    static unsigned long long int currentRtc();

    void connectToPlatform();
    void connectToDevices();

    bool registerDataProtocol(const std::string& protocol);
    void routePlatformData(const std::string& protocol, std::shared_ptr<Message> message);
    void routeDeviceData(const std::string& protocol, std::shared_ptr<Message> message);

    template <class P> bool registerDataProtocol();
    template <class P> void routePlatformData(std::shared_ptr<Message> message);
    template <class P> void routeDeviceData(std::shared_ptr<Message> message);

    Device m_device;

    std::unique_ptr<DeviceRepository> m_deviceRepository;

    std::shared_ptr<ConnectivityService> m_platformConnectivityService;
    std::shared_ptr<ConnectivityService> m_deviceConnectivityService;
    std::shared_ptr<Persistence> m_persistence;

    std::shared_ptr<InboundPlatformMessageHandler> m_inboundPlatformMessageHandler;
    std::shared_ptr<InboundDeviceMessageHandler> m_inboundDeviceMessageHandler;

    std::shared_ptr<OutboundServiceDataHandler> m_outboundServiceDataHandler;

    std::shared_ptr<ConnectivityFacade> m_platformConnectivityManager;
    std::shared_ptr<ConnectivityFacade> m_deviceConnectivityManager;

    std::shared_ptr<PublishingService> m_platformPublisher;
    std::shared_ptr<PublishingService> m_devicePublisher;

    // std::shared_ptr<FirmwareUpdateService> m_firmwareUpdateService;
    // std::shared_ptr<FileDownloadService> m_fileDownloadService;
    std::map<std::type_index, std::tuple<std::shared_ptr<DataServiceBase>, std::shared_ptr<ChannelProtocolResolver>>>
      m_dataServices;

    std::shared_ptr<DeviceRegistrationService> m_deviceRegistrationService;

    std::mutex m_lock;
    std::unique_ptr<CommandBuffer> m_commandBuffer;

    class ConnectivityFacade : public ConnectivityServiceListener
    {
    public:
        ConnectivityFacade(InboundMessageHandler& handler, std::function<void()> connectionLostHandler);

        void messageReceived(const std::string& topic, const std::string& message) override;
        void connectionLost() override;
        std::vector<std::string> getTopics() const override;

    private:
        InboundMessageHandler& m_messageHandler;
        std::function<void()> m_connectionLostHandler;
    };
};

template <class P> bool Wolk::registerDataProtocol()
{
    std::lock_guard<decltype(m_lock)> lg{m_lock};

    if (auto it = m_dataServices.find(typeid(P)) != m_dataServices.end())
    {
        LOG(INFO) << "Data protocol already registered";
        return true;
    }

    auto dataService = std::make_shared<DataService<P>>(m_device.getKey(), m_platformPublisher, m_devicePublisher);

    auto protocolResolver = std::make_shared<ChannelProtocolResolverImpl<P>>(
      *m_deviceRepository,
      [&](const std::string& protocol, std::shared_ptr<Message> message) { routePlatformData(protocol, message); },
      [&](const std::string& protocol, std::shared_ptr<Message> message) { routeDeviceData(protocol, message); });

    m_dataServices[typeid(P)] = std::make_pair(dataService, protocolResolver);

    m_inboundDeviceMessageHandler->setListener<P>(protocolResolver);
    m_inboundPlatformMessageHandler->setListener<P>(protocolResolver);

    return true;
}

template <> inline bool Wolk::registerDataProtocol<void>()
{
    return false;
}

template <class P> void Wolk::routePlatformData(std::shared_ptr<Message> message)
{
    std::lock_guard<decltype(m_lock)> lg{m_lock};

    auto it = m_dataServices.find(typeid(P));
    if (it != m_dataServices.end())
    {
        std::get<0>(it->second)->platformMessageReceived(message);
    }
    else
    {
        LOG(WARN) << "Message protocol not found for: " << message->getChannel();
    }
}

template <> inline void Wolk::routePlatformData<void>(std::shared_ptr<Message> message)
{
    LOG(WARN) << "Message protocol not found for: " << message->getChannel();
}

template <class P> void Wolk::routeDeviceData(std::shared_ptr<Message> message)
{
    std::lock_guard<decltype(m_lock)> lg{m_lock};

    auto it = m_dataServices.find(typeid(P));
    if (it != m_dataServices.end())
    {
        std::get<0>(it->second)->deviceMessageReceived(message);
    }
    else
    {
        LOG(WARN) << "Message protocol not found for: " << message->getChannel();
    }
}

template <> inline void Wolk::routeDeviceData<void>(std::shared_ptr<Message> message)
{
    LOG(WARN) << "Message protocol not found for: " << message->getChannel();
}

}    // namespace wolkabout

#endif
