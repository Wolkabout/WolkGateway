/*
 * Copyright 2019 WolkAbout Technology s.r.o.
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

#ifndef WOLKBUILDER_H
#define WOLKBUILDER_H

#include "ActuationHandler.h"
#include "ActuatorStatusProvider.h"
#include "ConfigurationHandler.h"
#include "ConfigurationProvider.h"
#include "FirmwareInstaller.h"
#include "connectivity/ConnectivityService.h"
#include "model/GatewayDevice.h"
#include "service/UrlFileDownloader.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace wolkabout
{
class Wolk;

class WolkBuilder final
{
public:
    /**
     * @brief WolkBuilder Initiates wolkabout::Wolk builder
     * @param device Device for which wolkabout::WolkBuilder is instantiated
     */
    WolkBuilder(GatewayDevice device);

    /**
     * @brief Allows passing of URI to custom WolkAbout IoT platform instance
     * @param host Server URI
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& platformHost(const std::string& host);

    /**
     * @brief Allows passing of server certificate
     * @param trust store
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& platformTrustStore(const std::string& trustStore);

    /**
     * @brief Allows passing of URI to custom local message bus
     * @param host Message Bus URI
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& gatewayHost(const std::string& host);

    /**
     * @brief Sets actuation handler
     * @param actuationHandler Lambda that handles actuation requests
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuationHandler(
      std::function<void(const std::string& reference, const std::string& value)> actuationHandler);

    /**
     * @brief Sets actuation handler
     * @param actuationHandler Instance of wolkabout::ActuationHandler
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuationHandler(std::shared_ptr<ActuationHandler> actuationHandler);

    /**
     * @brief Sets actuation status provider
     * @param actuatorStatusProvider Lambda that provides ActuatorStatus by reference of requested actuator
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuatorStatusProvider(
      std::function<ActuatorStatus(const std::string& reference)> actuatorStatusProvider);

    /**
     * @brief Sets actuation status provider
     * @param actuatorStatusProvider Instance of wolkabout::ActuatorStatusProvider
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuatorStatusProvider(std::shared_ptr<ActuatorStatusProvider> actuatorStatusProvider);

    /**
     * @brief Sets device configuration handler
     * @param configurationHandler Lambda that handles setting of configuration
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& configurationHandler(
      std::function<void(const std::vector<ConfigurationItem>& configuration)> configurationHandler);

    /**
     * @brief Sets device configuration handler
     * @param configurationHandler Instance of wolkabout::ConfigurationHandler that handles setting of configuration
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& configurationHandler(std::shared_ptr<ConfigurationHandler> configurationHandler);

    /**
     * @brief Sets device configuration provider
     * @param configurationProvider Lambda that provides device configuration
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& configurationProvider(std::function<std::vector<ConfigurationItem>()> configurationProvider);

    /**
     * @brief Sets device configuration provider
     * @param configurationProvider Instance of wolkabout::ConfigurationProvider that provides device configuration
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& configurationProvider(std::shared_ptr<ConfigurationProvider> configurationProvider);

    /**
     * @brief withFirmwareUpdate Enables firmware update for gateway
     * @param firmwareVersion Current version of the firmware
     * @param installer Instance of wolkabout::FirmwareInstaller used to install firmware
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withFirmwareUpdate(const std::string& firmwareVersion, std::shared_ptr<FirmwareInstaller> installer);

    /**
     * @brief withUrlFileDownload Enables downloading file from url
     * Url download must be enabled in GatewayDevice
     * @param urlDownloader Instance of wolkabout::UrlFileDownloader used to downlad firmware from provided url
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withUrlFileDownload(std::shared_ptr<UrlFileDownloader> urlDownloader);

    /**
     * @brief fileDownloadDirectory specifies directory where to download files
     * By default files are stored in the working directory of gateway
     * @param path Path to directory where files will be stored
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& fileDownloadDirectory(const std::string& path);

    /**
     * @brief withoutKeepAlive Disables ping mechanism used to notify WolkAbout IOT Platform
     * that device is still connected
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withoutKeepAlive();

    /**
     * @brief Builds Wolk instance
     * @return Wolk instance as std::unique_ptr<Wolk>
     *
     * @throws std::logic_error if device key is not present in wolkabout::Device
     * @throws std::logic_error if actuator status provider is not set, and wolkabout::Device has actuator references
     * @throws std::logic_error if actuation handler is not set, and wolkabout::Device has actuator references
     */
    std::unique_ptr<Wolk> build();

    /**
     * @brief operator std::unique_ptr<Wolk> Conversion to wolkabout::wolk as result returns std::unique_ptr to built
     * wolkabout::Wolk instance
     */
    operator std::unique_ptr<Wolk>();

private:
    std::string m_platformHost;
    std::string m_platformTrustStore = TRUST_STORE;
    std::string m_gatewayHost;
    GatewayDevice m_device;

    std::function<void(std::string, std::string)> m_actuationHandlerLambda;
    std::shared_ptr<ActuationHandler> m_actuationHandler;

    std::function<ActuatorStatus(std::string)> m_actuatorStatusProviderLambda;
    std::shared_ptr<ActuatorStatusProvider> m_actuatorStatusProvider;

    std::function<void(const std::vector<ConfigurationItem>& configuration)> m_configurationHandlerLambda;
    std::shared_ptr<ConfigurationHandler> m_configurationHandler;

    std::function<std::vector<ConfigurationItem>()> m_configurationProviderLambda;
    std::shared_ptr<ConfigurationProvider> m_configurationProvider;

    std::string m_fileDownloadDirectory = ".";

    std::string m_firmwareVersion;
    std::shared_ptr<FirmwareInstaller> m_firmwareInstaller;

    std::shared_ptr<UrlFileDownloader> m_urlFileDownloader;

    bool m_keepAliveEnabled;

    static const constexpr char* WOLK_DEMO_HOST = "ssl://api-demo.wolkabout.com:8883";
    static const constexpr char* MESSAGE_BUS_HOST = "tcp://localhost:1883";
    static const constexpr char* TRUST_STORE = "ca.crt";
    static const constexpr char* DATABASE = "deviceRepository.db";
};
}    // namespace wolkabout

#endif
