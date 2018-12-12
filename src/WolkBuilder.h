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

#ifndef WOLKBUILDER_H
#define WOLKBUILDER_H

#include "FirmwareInstaller.h"
#include "connectivity/ConnectivityService.h"
#include "model/Device.h"
#include "service/UrlFileDownloader.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace wolkabout
{
class GatewayDataProtocol;
class Wolk;

class WolkBuilder final
{
public:
    /**
     * @brief WolkBuilder Initiates wolkabout::Wolk builder
     * @param device Device for which wolkabout::WolkBuilder is instantiated
     */
    WolkBuilder(Device device);

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
     * @brief withDataProtocol Defines which data protocol to use
     * @tparam Protocol protocol type to register
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withDataProtocol(std::shared_ptr<GatewayDataProtocol> protocol);

    /**
     * @brief withFirmwareUpdate Enables firmware update for gateway
     * @param firmwareVersion Current version of the firmware
     * @param installer Instance of wolkabout::FirmwareInstaller used to install firmware
     * @param firmwareDownloadDirectory Directory where to download firmware file
     * @param maxFirmwareFileSize Maximum size of firmware file that can be handled
     * @param urlDownloader Instance of wolkabout::UrlFileDownloader used to downlad firmware from provided url
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withFirmwareUpdate(const std::string& firmwareVersion, std::shared_ptr<FirmwareInstaller> installer,
                                    const std::string& firmwareDownloadDirectory,
                                    std::uint_fast64_t maxFirmwareFileSize, std::uint_fast64_t maxFirmwareFileChunkSize,
                                    std::shared_ptr<UrlFileDownloader> urlDownloader = nullptr);

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
    Device m_device;

    std::shared_ptr<GatewayDataProtocol> m_dataProtocol;

    std::string m_firmwareVersion;
    std::string m_firmwareDownloadDirectory = "";
    std::uint_fast64_t m_maxFirmwareFileSize = 10 * 1024 * 1024;
    std::uint_fast64_t m_maxFirmwareFileChunkSize = 10 * 1024;
    std::shared_ptr<FirmwareInstaller> m_firmwareInstaller;
    std::shared_ptr<UrlFileDownloader> m_urlFileDownloader;

    bool m_keepAliveEnabled;

    static const constexpr char* WOLK_DEMO_HOST = "ssl://api-demo.wolkabout.com:8883";
    static const constexpr char* MESSAGE_BUS_HOST = "tcp://localhost:1883";
    static const constexpr char* TRUST_STORE = "ca.crt";
};
}    // namespace wolkabout

#endif
