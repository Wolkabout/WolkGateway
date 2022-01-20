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

#ifndef WOLKBUILDER_H
#define WOLKBUILDER_H

#include "core/model/Device.h"
#include "core/persistence/Persistence.h"
#include "core/protocol/DataProtocol.h"
#include "core/protocol/ErrorProtocol.h"
#include "core/protocol/FileManagementProtocol.h"
#include "core/protocol/FirmwareUpdateProtocol.h"
#include "core/protocol/GatewaySubdeviceProtocol.h"
#include "core/protocol/PlatformStatusProtocol.h"
#include "core/protocol/RegistrationProtocol.h"
#include "gateway/api/DataProvider.h"
#include "gateway/persistence/GatewayPersistence.h"
#include "wolk/WolkInterfaceType.h"
#include "wolk/api/FeedUpdateHandler.h"
#include "wolk/api/FileListener.h"
#include "wolk/api/FirmwareInstaller.h"
#include "wolk/api/FirmwareParametersListener.h"
#include "wolk/api/ParameterHandler.h"
#include "wolk/api/PlatformStatusListener.h"
#include "wolk/service/file_management/FileDownloader.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace wolkabout
{
namespace gateway
{
class WolkGateway;

class WolkBuilder final
{
public:
    /**
     * @brief WolkBuilder Initiates wolkabout::Wolk builder
     * @param device Device for which wolkabout::WolkBuilder is instantiated
     */
    explicit WolkBuilder(Device device);

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
     * @brief Sets feed update handler
     * @param feedUpdateHandler Lambda that handles feed update requests. Will receive a map of readings grouped by the
     * time when the update happened. Key is a timestamp in milliseconds, and value is a vector of readings that changed
     * at that time.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& feedUpdateHandler(
      const std::function<void(std::string, const std::map<std::uint64_t, std::vector<Reading>>)>& feedUpdateHandler);

    /**
     * @brief Sets feed update handler
     * @param feedUpdateHandler Instance of wolkabout::FeedUpdateHandler
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& feedUpdateHandler(std::weak_ptr<connect::FeedUpdateHandler> feedUpdateHandler);

    /**
     * @brief Sets parameter handler
     * @param parameterHandlerLambda Lambda that handles parameters updates
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& parameterHandler(
      const std::function<void(std::string, std::vector<Parameter>)>& parameterHandlerLambda);

    /**
     * @brief Sets parameter handler
     * @param parameterHandler Instance of wolkabout::ParameterHandler
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& parameterHandler(std::weak_ptr<connect::ParameterHandler> parameterHandler);

    /**
     * @brief Sets underlying persistence mechanism to be used<br>
     *        Sample in-memory persistence is used as default
     * @param persistence std::shared_ptr to wolkabout::Persistence implementation
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withPersistence(std::unique_ptr<GatewayPersistence> persistence);

    /**
     * @brief withDataProtocol Defines which data protocol to use
     * @param Protocol unique_ptr to wolkabout::DataProtocol implementation
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withDataProtocol(std::unique_ptr<DataProtocol> protocol);

    /**
     * @brief withErrorProtocol Defines which error protocol to use
     * @param errorRetainTime The time defining how long will the error messages be retained. The default retain time is
     * 1s (1000ms).
     * @param protocol Unique_ptr to wolkabout::ErrorProtocol implementation (providing nullptr will still refer to
     * default one)
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withErrorProtocol(std::chrono::milliseconds errorRetainTime,
                                   std::unique_ptr<ErrorProtocol> protocol = nullptr);

    /**
     * @brief Sets the Wolk module to allow file management functionality.
     * @details This one is meant to enable the File Transfer, but not File URL Download.
     * @param fileDownloadLocation The folder location for file management.
     * @param maxPacketSize The maximum packet size for downloading chunks (in KBs).
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withFileTransfer(const std::string& fileDownloadLocation, std::uint64_t maxPacketSize = 268435);

    /**
     * @brief Sets the Wolk module to allow file management functionality.
     * @details This one is meant to enable File URL Download, but can enabled File Transfer too.
     * @param fileDownloadLocation The folder location for file management.
     * @param fileDownloader The implementation that will download the files.
     * @param transferEnabled Whether the File Transfer should be enabled too.
     * @param maxPacketSize The max packet size for downloading chunks (in MBs).
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withFileURLDownload(const std::string& fileDownloadLocation,
                                     std::shared_ptr<connect::FileDownloader> fileDownloader = nullptr,
                                     bool transferEnabled = false, std::uint64_t maxPacketSize = 268435);

    /**
     * @brief Sets the Wolk module file listener.
     * @details This object will receive information about newly obtained or removed files. It will be used with
     * `withFileListener` when the service gets created.
     * @param fileListener A pointer to the instance of the file listener.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withFileListener(const std::shared_ptr<connect::FileListener>& fileListener);

    /**
     * @brief Sets the Wolk module to allow firmware update functionality.
     * @details This one is meant for PUSH configuration, where the functionality is implemented using the
     * `FirmwareInstaller`. This object will received instructions from the platform of when to install new firmware.
     * @param firmwareInstaller The implementation of the FirmwareInstaller interface.
     * @param workingDirectory The directory where the session file will be kept.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withFirmwareUpdate(std::unique_ptr<connect::FirmwareInstaller> firmwareInstaller,
                                    const std::string& workingDirectory = "./");

    /**
     * @brief Sets the Wolk module to allow firmware update functionality.
     * @details This one is meant for PULL configuration, where the functionality is implemented using the
     * `FirmwareParametersListener`. This object will receive information from the platform of when and where to check
     * for firmware updates.
     * @param firmwareParametersListener The implementation of the FirmwareParametersListener interface.
     * @param workingDirectory The directory where the session file will be kept.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withFirmwareUpdate(std::unique_ptr<connect::FirmwareParametersListener> firmwareParametersListener,
                                    const std::string& workingDirectory = "./");

    /**
     * @brief Sets the seconds the MQTT connection with the platform will be kept alive for.
     * @param keepAlive The amount of seconds the connection will be pinged.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& setMqttKeepAlive(std::uint16_t keepAlive);

    /**
     * @brief Builds Wolk instance
     * @return Wolk instance as std::unique_ptr<Wolk>
     *
     * @throws std::logic_error if device key is not present in wolkabout::Device
     * @throws std::logic_error if actuator status provider is not set, and wolkabout::Device has actuator references
     * @throws std::logic_error if actuation handler is not set, and wolkabout::Device has actuator references
     */
    std::unique_ptr<WolkGateway> build();

    /**
     * @brief operator std::unique_ptr<Wolk> Conversion to wolkabout::wolk as result returns std::unique_ptr to built
     * wolkabout::Wolk instance
     */
    operator std::unique_ptr<WolkGateway>();

private:
    // The gateway device information
    Device m_device;

    // Here we store the connection information for the connectivity services.
    std::string m_platformHost;
    std::string m_platformTrustStore;
    std::uint16_t m_platformMqttKeepAliveSec;
    std::string m_gatewayHost;

    // Here is the place for external entities capable of receiving Reading values.
    std::function<void(std::string, std::map<std::uint64_t, std::vector<Reading>>)> m_feedUpdateHandlerLambda;
    std::weak_ptr<connect::FeedUpdateHandler> m_feedUpdateHandler;

    // Here is the place for external entities capable of receiving Parameter values.
    std::function<void(std::string, std::vector<Parameter>)> m_parameterHandlerLambda;
    std::weak_ptr<connect::ParameterHandler> m_parameterHandler;

    // Place for the gateway persistence
    std::unique_ptr<GatewayPersistence> m_gatewayPersistence;

    // Here is the place for all the protocols that are being held
    std::unique_ptr<DataProtocol> m_dataProtocol;
    std::unique_ptr<ErrorProtocol> m_errorProtocol;
    std::chrono::milliseconds m_errorRetainTime;
    std::unique_ptr<FileManagementProtocol> m_fileManagementProtocol;
    std::unique_ptr<FirmwareUpdateProtocol> m_firmwareUpdateProtocol;
    std::unique_ptr<GatewaySubdeviceProtocol> m_gatewaySubdeviceProtocol;
    std::unique_ptr<PlatformStatusProtocol> m_platformStatusProtocol;

    // Here is the place for all the file transfer related parameters
    std::shared_ptr<connect::FileDownloader> m_fileDownloader;
    std::string m_fileDownloadDirectory;
    bool m_fileTransferEnabled;
    bool m_fileTransferUrlEnabled;
    std::uint64_t m_maxPacketSize;
    std::shared_ptr<connect::FileListener> m_fileListener;

    // Here is the place for all the firmware update related parameters
    std::unique_ptr<connect::FirmwareInstaller> m_firmwareInstaller;
    std::string m_workingDirectory;
    std::unique_ptr<connect::FirmwareParametersListener> m_firmwareParametersListener;

    // TODO Add gateway specific things

    // These are the default values that are going to be used for the connection parameters
    static const constexpr char* WOLK_DEMO_HOST = "ssl://api-demo.wolkabout.com:8883";
    static const constexpr char* MESSAGE_BUS_HOST = "tcp://localhost:1883";
    static const constexpr char* TRUST_STORE = "ca.crt";
    static const constexpr std::uint64_t MAX_PACKET_SIZE = 268434;
    static const constexpr char* DATABASE = "deviceRepository.db";
};
}    // namespace gateway
}    // namespace wolkabout

#endif
