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
#include "GatewayInboundDeviceMessageHandler.h"
#include "WolkBuilder.h"
#include "model/Device.h"
#include "protocol/GatewayDataProtocol.h"
#include "protocol/GatewayDeviceRegistrationProtocol.h"
#include "protocol/GatewayStatusProtocol.h"
#include "repository/DeviceRepository.h"
#include "repository/ExistingDevicesRepository.h"
#include "service/DataService.h"
#include "service/DeviceStatusService.h"
#include "service/PublishingService.h"
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
class DeviceManager;
class OutboundServiceDataHandler;
class DataServiceBase;
class DeviceRegistrationService;
class KeepAliveService;
class StatusMessageRouter;

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
    static const constexpr std::chrono::seconds KEEP_ALIVE_INTERVAL{600};

    Wolk(Device device);

    void addToCommandBuffer(std::function<void()> command);

    static unsigned long long int currentRtc();

    void notifyPlatformConnected();
    void notifyPlatformDisonnected();
    void notifyDevicesConnected();
    void notifyDevicesDisonnected();

    void connectToPlatform();
    void connectToDevices();

    void routePlatformData(const std::string& protocol, std::shared_ptr<Message> message);
    void routeDeviceData(const std::string& protocol, std::shared_ptr<Message> message);

    void gatewayRegistered();
    void setupGatewayListeners(const std::string& protocol);

    void registerDataProtocol(std::shared_ptr<GatewayDataProtocol> protocol);

    Device m_device;

    std::unique_ptr<GatewayStatusProtocol> m_statusProtocol;
    std::unique_ptr<GatewayDeviceRegistrationProtocol> m_registrationProtocol;

    std::unique_ptr<DeviceRepository> m_deviceRepository;
    std::unique_ptr<ExistingDevicesRepository> m_existingDevicesRepository;

    std::unique_ptr<ConnectivityService> m_platformConnectivityService;
    std::unique_ptr<ConnectivityService> m_deviceConnectivityService;

    std::unique_ptr<InboundPlatformMessageHandler> m_inboundPlatformMessageHandler;
    std::unique_ptr<InboundDeviceMessageHandler> m_inboundDeviceMessageHandler;

    std::unique_ptr<PublishingService> m_platformPublisher;
    std::unique_ptr<PublishingService> m_devicePublisher;

    std::map<std::string, std::tuple<std::shared_ptr<DataService>, std::shared_ptr<GatewayDataProtocol>,
                                     std::shared_ptr<ChannelProtocolResolver>>>
      m_dataServices;

    std::shared_ptr<DeviceRegistrationService> m_deviceRegistrationService;
    std::shared_ptr<KeepAliveService> m_keepAliveService;

    std::shared_ptr<DeviceStatusService> m_deviceStatusService;
    std::shared_ptr<StatusMessageRouter> m_statusMessageRouter;

    std::mutex m_lock;
    std::unique_ptr<CommandBuffer> m_commandBuffer;

    template <class MessageHandler> class ConnectivityFacade : public ConnectivityServiceListener
    {
    public:
        ConnectivityFacade(MessageHandler& handler, std::function<void()> connectionLostHandler);

        void messageReceived(const std::string& channel, const std::string& message) override;
        void connectionLost() override;
        std::vector<std::string> getChannels() const override;

    private:
        MessageHandler& m_messageHandler;
        std::function<void()> m_connectionLostHandler;
    };

    std::shared_ptr<ConnectivityFacade<InboundPlatformMessageHandler>> m_platformConnectivityManager;
    std::shared_ptr<ConnectivityFacade<InboundDeviceMessageHandler>> m_deviceConnectivityManager;
};

template <class MessageHandler>
Wolk::ConnectivityFacade<MessageHandler>::ConnectivityFacade(MessageHandler& handler,
                                                             std::function<void()> connectionLostHandler)
: m_messageHandler{handler}, m_connectionLostHandler{connectionLostHandler}
{
}

template <class MessageHandler>
void Wolk::ConnectivityFacade<MessageHandler>::messageReceived(const std::string& channel, const std::string& message)
{
    m_messageHandler.messageReceived(channel, message);
}

template <class MessageHandler> void Wolk::ConnectivityFacade<MessageHandler>::connectionLost()
{
    m_connectionLostHandler();
}

template <class MessageHandler> std::vector<std::string> Wolk::ConnectivityFacade<MessageHandler>::getChannels() const
{
    return m_messageHandler.getChannels();
}
}    // namespace wolkabout

#endif
