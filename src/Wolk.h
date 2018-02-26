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

#include "InboundDeviceMessageHandler.h"
#include "WolkBuilder.h"
#include "model/Device.h"
#include "service/DataService.h"
#include "utilities/CommandBuffer.h"
#include "utilities/StringUtils.h"

#include <functional>
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
class PublishingService;
class OutboundServiceDataHandler;
class DataServiceBase;

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

    template <class P> bool registerDataProtocol();

    Device m_device;

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
    std::vector<std::shared_ptr<DataServiceBase>> m_dataServices;

    std::unique_ptr<CommandBuffer> m_commandBuffer;

    std::map<std::type_index, std::vector<std::string>> m_protocolTopics;


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
    // check if protocol is already registered
    auto it = m_protocolTopics.find(typeid(P));
    if (it != m_protocolTopics.end())
    {
        LOG(DEBUG) << "Protocol already registered";
        return true;
    }

    // check if any topics are in confilict
    for (const auto& kvp : m_protocolTopics)
    {
        for (const auto& registeredTopic : kvp.second)
        {
            for (const auto topic : P::getInstance().getDeviceTopics())
            {
                if (StringUtils::mqttTopicMatch(registeredTopic, topic))
                {
                    LOG(WARN) << "Conflicted protocol topics: " << registeredTopic << ", " << topic;
                    return false;
                }
            }

            for (const auto topic : P::getInstance().getPlatformTopics())
            {
                if (StringUtils::mqttTopicMatch(registeredTopic, topic))
                {
                    LOG(WARN) << "Conflicted protocol topics: " << registeredTopic << ", " << topic;
                    return false;
                }
            }
        }
    }

    // add topics to list
    std::vector<std::string> newTopics = P::getInstance().getDeviceTopics();
    const auto platformTopics = P::getInstance().getPlatformTopics();
    newTopics.reserve(newTopics.size() + platformTopics.size());
    newTopics.insert(newTopics.end(), platformTopics.begin(), platformTopics.end());

    m_protocolTopics[typeid(P)] = newTopics;

	auto dataService =
			std::make_shared<DataService<P>>(m_device.getKey(), m_platformPublisher, m_devicePublisher);

    m_dataServices.push_back(dataService);

    m_inboundDeviceMessageHandler->setListener<P>(dataService);
    m_inboundPlatformMessageHandler->setListener<P>(dataService);

    return true;
}

template <> inline bool Wolk::registerDataProtocol<void>()
{
    return false;
}
}    // namespace wolkabout

#endif
