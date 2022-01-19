/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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

#include "core/model/Device.h"
#include "core/utilities/StringUtils.h"
#include "gateway/WolkBuilder.h"
#include "wolk/WolkSingle.h"

#include <algorithm>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
namespace gateway
{
class FeedUpdateHandler;
// class CommandBuffer;
// class ConfigurationSetCommand;
// class ConnectivityService;
// class DataProtocol;
// class DataService;
// class DeviceRepository;
// class DeviceStatusService;
// class ExistingDevicesRepository;
// class FileDownloadService;
// class FileRepository;
// class FirmwareUpdateService;
// class GatewayDataProtocol;
// class GatewayDataService;
// class GatewayFirmwareUpdateProtocol;
// class GatewayStatusProtocol;
// class GatewaySubdeviceRegistrationProtocol;
// class GatewayUpdateService;
// class InboundDeviceMessageHandler;
// class InboundPlatformMessageHandler;
// class JsonDFUProtocol;
// class JsonDownloadProtocol;
// class KeepAliveService;
// class PublishingService;
// class Persistence;
// class PlatformStatusService;
// class RegistrationMessageRouter;
// class RegistrationProtocol;
// class StatusMessageRouter;
// class StatusProtocol;
// class SubdeviceRegistrationService;

class Wolk
{
    friend class WolkBuilder;

public:
    virtual ~Wolk();

    /**
     * @brief Initiates wolkabout::WolkBuilder that configures device to connect to WolkAbout IoT Cloud
     * @param device wolkabout::Device
     * @return wolkabout::WolkBuilder instance
     */
    static WolkBuilder newBuilder(Device device);

    /**
     * This is the default getter method for obtaining the platform connection status.
     *
     * @return Platform connection status.
     */
    virtual bool isConnectedToPlatform();

    /**
     * This is the default setter method for setting a callback function that listens to the platform connection status.
     *
     * @param platformConnectionStatusListener The callback function.
     */
    virtual void setPlatformConnectionStatusListener(const std::function<void(bool)>& platformConnectionStatusListener);

    /**
     * @brief Publishes reading to WolkAbout IoT Cloud
     * @param reading Reading
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    void addReading(const Reading& reading, std::uint64_t rtc = 0);

    /**
     * @brief Publishes reading to WolkAbout IoT Cloud
     * @param reference Feed reference
     * @param value Feed value
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    template <typename T> void addReading(const std::string& reference, const T& value, std::uint64_t rtc = 0);

    /**
     * @brief Publishes reading to WolkAbout IoT Cloud
     * @param reference Feed reference
     * @param value Feed value
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    void addReading(const std::string& reference, const std::string& value, std::uint64_t rtc = 0);

    /**
     * @brief Publishes multi-value reading to WolkAbout IoT Cloud
     * @param reference Feed reference
     * @param values Multi-value feed values
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    template <typename T>
    void addReading(const std::string& reference, const std::vector<T>& values, std::uint64_t rtc = 0);

    /**
     * @brief Publishes multi-value reading to WolkAbout IoT Cloud
     * @param reference Feed reference
     * @param values Multi-value feed values
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    void addReading(const std::string& reference, const std::vector<std::string>& values, std::uint64_t rtc = 0);

    void pullFeedValues();
    void pullParameters();

    void registerFeed(const Feed& feed);
    void registerFeeds(const std::vector<Feed>& feeds);

    void removeFeed(const std::string& reference);
    void removeFeeds(const std::vector<std::string>& references);

    void addAttribute(Attribute attribute);

    void updateParameter(Parameter parameters);

    /**
     * @brief connect Establishes connection with WolkAbout IoT platform
     */
    virtual void connect() = 0;

    /**
     * @brief disconnect Disconnects from WolkAbout IoT platform
     */
    virtual void disconnect() = 0;

    /**
     * @brief Publishes data
     */
    void publish();

protected:
    explicit Wolk(Device device);

    void addToCommandBuffer(std::function<void()> command);

    static std::uint64_t currentRtc();

    void flushFeeds();

    // callbacks
    void handleFeedUpdate(const std::map<std::uint64_t, std::vector<Reading>>& readings);
    void handleParameterUpdate(const std::vector<Parameter> parameters);

    void platformDisconnected();

    void gatewayUpdated();
    //

    void publishEverything();
    void publishFirmwareStatus();
    void publishFileList();
    void updateGatewayAndDeleteDevices();

    void notifyPlatformConnected();
    void notifyPlatformDisonnected();

    void connectToPlatform(bool firstTime = false);

    void requestActuatorStatusesForDevices();
    void requestActuatorStatusesForDevice(const std::string& deviceKey);

    Device m_device;

    std::atomic<bool> m_connected;
    std::function<void(bool)> m_platformConnectionStatusListener;

    std::unique_ptr<DeviceRepository> m_deviceRepository;
    std::unique_ptr<ExistingDevicesRepository> m_existingDevicesRepository;
    std::unique_ptr<FileRepository> m_fileRepository;

    std::unique_ptr<Persistence> m_gatewayPersistence;

    std::unique_ptr<ConnectivityService> m_platformConnectivityService;
    std::unique_ptr<ConnectivityService> m_deviceConnectivityService;

    std::unique_ptr<InboundPlatformMessageHandler> m_inboundPlatformMessageHandler;
    std::unique_ptr<InboundDeviceMessageHandler> m_inboundDeviceMessageHandler;

    std::unique_ptr<PublishingService> m_platformPublisher;
    std::unique_ptr<PublishingService> m_devicePublisher;

    std::unique_ptr<DataProtocol> m_dataProtocol;
    std::unique_ptr<GatewayDataProtocol> m_gatewayDataProtocol;
    std::unique_ptr<GatewayDataService> m_gatewayDataService;
    std::shared_ptr<DataService> m_dataService;

    std::unique_ptr<RegistrationProtocol> m_registrationProtocol;
    std::unique_ptr<GatewaySubdeviceRegistrationProtocol> m_gatewayRegistrationProtocol;
    std::unique_ptr<GatewayUpdateService> m_gatewayUpdateService;
    std::unique_ptr<SubdeviceRegistrationService> m_subdeviceRegistrationService;
    std::shared_ptr<RegistrationMessageRouter> m_registrationMessageRouter;

    std::unique_ptr<StatusProtocol> m_statusProtocol;
    std::unique_ptr<GatewayStatusProtocol> m_gatewayStatusProtocol;
    std::unique_ptr<KeepAliveService> m_keepAliveService;
    std::unique_ptr<DeviceStatusService> m_deviceStatusService;
    std::shared_ptr<StatusMessageRouter> m_statusMessageRouter;
    std::shared_ptr<PlatformStatusService> m_platformStatusService;

    std::unique_ptr<JsonDFUProtocol> m_firmwareUpdateProtocol;
    std::unique_ptr<GatewayFirmwareUpdateProtocol> m_gatewayFirmwareUpdateProtocol;
    std::shared_ptr<FirmwareUpdateService> m_firmwareUpdateService;

    std::unique_ptr<JsonDownloadProtocol> m_fileDownloadProtocol;
    std::shared_ptr<FileDownloadService> m_fileDownloadService;

    std::function<void(std::map<std::uint64_t, std::vector<Reading>>)> m_feedHandlerLambda;
    std::shared_ptr<FeedUpdateHandler> m_feedHandler;

    std::function<ActuatorStatus(std::string)> m_actuatorStatusProviderLambda;
    std::weak_ptr<ActuatorStatusProvider> m_actuatorStatusProvider;

    std::function<void(const std::vector<ConfigurationItem>& configuration)> m_configurationHandlerLambda;
    std::weak_ptr<ConfigurationHandler> m_configurationHandler;

    std::function<std::vector<ConfigurationItem>()> m_configurationProviderLambda;
    std::weak_ptr<ConfigurationProvider> m_configurationProvider;

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

template <typename T> void Wolk::addReading(const std::string& reference, const T& value, std::uint64_t rtc)
{
    addReading(reference, StringUtils::toString(value), rtc);
}

template <typename T>
void Wolk::addReading(const std::string& reference, const std::vector<T>& values, std::uint64_t rtc)
{
    std::vector<std::string> stringifiedValues(values.size());
    std::transform(values.cbegin(), values.cend(), stringifiedValues.begin(),
                   [&](const T& value) -> std::string { return StringUtils::toString(value); });

    addReading(reference, stringifiedValues, rtc);
}

template <class MessageHandler>
Wolk::ConnectivityFacade<MessageHandler>::ConnectivityFacade(MessageHandler& handler,
                                                             std::function<void()> connectionLostHandler)
: m_messageHandler{handler}, m_connectionLostHandler{std::move(connectionLostHandler)}
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
}    // namespace gateway
}    // namespace wolkabout

#endif
