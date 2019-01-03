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

#include "ActuationHandler.h"
#include "ActuatorStatusProvider.h"
#include "ChannelProtocolResolver.h"
#include "ConfigurationHandler.h"
#include "ConfigurationProvider.h"
#include "GatewayInboundDeviceMessageHandler.h"
#include "WolkBuilder.h"
#include "model/Device.h"
#include "persistence/inmemory/InMemoryPersistence.h"
#include "protocol/DataProtocol.h"
#include "protocol/GatewayDataProtocol.h"
#include "protocol/GatewayDeviceRegistrationProtocol.h"
#include "protocol/GatewayFileDownloadProtocol.h"
#include "protocol/GatewayFirmwareUpdateProtocol.h"
#include "protocol/GatewayStatusProtocol.h"
#include "repository/DeviceRepository.h"
#include "repository/ExistingDevicesRepository.h"
#include "service/DataService.h"
#include "service/DeviceStatusService.h"
#include "service/GatewayDataService.h"
#include "service/PublishingService.h"
#include "utilities/CommandBuffer.h"
#include "utilities/StringUtils.h"

#include <algorithm>
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
class FileDownloadService;
class FirmwareUpdateService;
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

    /**
     * @brief Publishes sensor reading to WolkAbout IoT Cloud<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     * @param reference Sensor reference
     * @param value Sensor value<br>
     *              Supported types:<br>
     *               - bool<br>
     *               - float<br>
     *               - double<br>
     *               - signed int<br>
     *               - signed long int<br>
     *               - signed long long int<br>
     *               - unsigned int<br>
     *               - unsigned long int<br>
     *               - unsigned long long int<br>
     *               - string<br>
     *               - char*<br>
     *               - const char*<br>
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    template <typename T> void addSensorReading(const std::string& reference, T value, unsigned long long int rtc = 0);

    /**
     * @brief Publishes sensor reading to WolkAbout IoT Cloud<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     * @param reference Sensor reference
     * @param value Sensor value
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    void addSensorReading(const std::string& reference, std::string value, unsigned long long int rtc = 0);

    /**
     * @brief Publishes multi-value sensor reading to WolkAbout IoT Cloud<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     * @param reference Sensor reference
     * @param values Multi-value sensor values<br>
     *              Supported types:<br>
     *               - bool<br>
     *               - float<br>
     *               - double<br>
     *               - signed int<br>
     *               - signed long int<br>
     *               - signed long long int<br>
     *               - unsigned int<br>
     *               - unsigned long int<br>
     *               - unsigned long long int<br>
     *               - string<br>
     *               - char*<br>
     *               - const char*<br>
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    template <typename T>
    void addSensorReading(const std::string& reference, std::initializer_list<T> values,
                          unsigned long long int rtc = 0);

    /**
     * @brief Publishes multi-value sensor reading to WolkAbout IoT Cloud<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     * @param reference Sensor reference
     * @param values Multi-value sensor values<br>
     *              Supported types:<br>
     *               - bool<br>
     *               - float<br>
     *               - double<br>
     *               - signed int<br>
     *               - signed long int<br>
     *               - signed long long int<br>
     *               - unsigned int<br>
     *               - unsigned long int<br>
     *               - unsigned long long int<br>
     *               - string<br>
     *               - char*<br>
     *               - const char*<br>
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    template <typename T>
    void addSensorReading(const std::string& reference, const std::vector<T> values, unsigned long long int rtc = 0);

    /**
     * @brief Publishes multi-value sensor reading to WolkAbout IoT Cloud<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     * @param reference Sensor reference
     * @param values Multi-value sensor values
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    void addSensorReading(const std::string& reference, const std::vector<std::string> values,
                          unsigned long long int rtc = 0);

    /**
     * @brief Publishes alarm to WolkAbout IoT Cloud<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     * @param reference Alarm reference
     * @param active Is alarm active or not
     * @param rtc POSIX time at which event occurred - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    void addAlarm(const std::string& reference, bool active, unsigned long long int rtc = 0);

    /**
     * @brief Invokes ActuatorStatusProvider to obtain actuator status, and the publishes it.<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     * @param Actuator reference
     */
    void publishActuatorStatus(const std::string& reference);

    /**
     * @brief Invokes ConfigurationProvider to obtain device configuration, and the publishes it.<br>
     *        This method is thread safe, and can be called from multiple thread simultaneously
     */
    void publishConfiguration();

    /**
     * @brief Publishes data
     */
    void publish();

private:
    static const constexpr std::chrono::seconds KEEP_ALIVE_INTERVAL{600};

    Wolk(Device device);

    void addToCommandBuffer(std::function<void()> command);

    static unsigned long long int currentRtc();

    void flushActuatorStatuses();
    void flushAlarms();
    void flushSensorReadings();
    void flushConfiguration();

    void handleActuatorSetCommand(const std::string& reference, const std::string& value);
    void handleActuatorGetCommand(const std::string& reference);

    void handleConfigurationSetCommand(const ConfigurationSetCommand& command);
    void handleConfigurationGetCommand();

    void publishFirmwareStatus();

    std::string getSensorDelimiter(const std::string& reference);
    std::map<std::string, std::string> getConfigurationDelimiters();

    void notifyPlatformConnected();
    void notifyPlatformDisonnected();
    void notifyDevicesConnected();
    void notifyDevicesDisonnected();

    void connectToPlatform();
    void connectToDevices();

    void routePlatformData(const std::string& protocol, std::shared_ptr<Message> message);
    void routeDeviceData(const std::string& protocol, std::shared_ptr<Message> message);

    void registerDataProtocol(std::shared_ptr<GatewayDataProtocol> protocol,
                              std::shared_ptr<DataService> dataService = nullptr);

    void requestActuatorStatusesForDevices();
    void requestActuatorStatusesForDevice(const std::string& deviceKey);

    Device m_device;

    std::unique_ptr<GatewayStatusProtocol> m_statusProtocol;
    std::unique_ptr<GatewayDeviceRegistrationProtocol> m_registrationProtocol;
    std::unique_ptr<GatewayFileDownloadProtocol> m_fileDownloadProtocol;
    std::unique_ptr<GatewayFirmwareUpdateProtocol> m_firmwareUpdateProtocol;

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

    std::unique_ptr<GatewayDataService> m_gatewayDataService;
    std::unique_ptr<Persistence> m_gatewayPersistence;
    std::unique_ptr<DataProtocol> m_gatewayDataProtocol;

    std::shared_ptr<DeviceRegistrationService> m_deviceRegistrationService;
    std::shared_ptr<KeepAliveService> m_keepAliveService;

    std::shared_ptr<DeviceStatusService> m_deviceStatusService;
    std::shared_ptr<StatusMessageRouter> m_statusMessageRouter;

    std::shared_ptr<FirmwareUpdateService> m_firmwareUpdateService;
    std::shared_ptr<FileDownloadService> m_fileDownloadService;

    std::function<void(std::string, std::string)> m_actuationHandlerLambda;
    std::weak_ptr<ActuationHandler> m_actuationHandler;

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

template <typename T> void Wolk::addSensorReading(const std::string& reference, T value, unsigned long long rtc)
{
    addSensorReading(reference, StringUtils::toString(value), rtc);
}

template <typename T>
void Wolk::addSensorReading(const std::string& reference, std::initializer_list<T> values, unsigned long long int rtc)
{
    addSensorReading(reference, std::vector<T>(values), rtc);
}

template <typename T>
void Wolk::addSensorReading(const std::string& reference, const std::vector<T> values, unsigned long long int rtc)
{
    std::vector<std::string> stringifiedValues(values.size());
    std::transform(values.cbegin(), values.cend(), stringifiedValues.begin(),
                   [&](const T& value) -> std::string { return StringUtils::toString(value); });

    addSensorReading(reference, stringifiedValues, rtc);
}

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
