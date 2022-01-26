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
#include "core/persistence/MessagePersistence.h"
#include "core/persistence/Persistence.h"
#include "core/protocol/DataProtocol.h"
#include "core/protocol/ErrorProtocol.h"
#include "core/protocol/FileManagementProtocol.h"
#include "core/protocol/FirmwareUpdateProtocol.h"
#include "core/protocol/GatewayPlatformStatusProtocol.h"
#include "core/protocol/GatewayRegistrationProtocol.h"
#include "core/protocol/GatewaySubdeviceProtocol.h"
#include "core/protocol/PlatformStatusProtocol.h"
#include "core/protocol/RegistrationProtocol.h"
#include "gateway/api/DataProvider.h"
#include "gateway/repository/device/DeviceRepository.h"
#include "gateway/repository/existing_device/ExistingDevicesRepository.h"
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

/**
 * This enumeration describes the policy of storing device data for the sake of message filtering.
 */
enum class DeviceStoragePolicy
{
    NONE,    // No device information will be stored, and no filtering will be applied - no memory/storage usage, most
             // network usage.
    CACHED,    // Only cache memory will be used, no data will be persisted - no storage usage, higher memory usage and
               // higher usage of resources on startup, but fast filtering
    PERSISTENT,    // Only persistent storage will be used, no cache memory will be used - no memory usage, persistent
                   // storage required and high usage of resources only on first startup, and slow filtering
    FULL    // Will use combination of both `CACHED` and `PERSISTENT` to gain both quick filtering, and low usage of
            // resources on startup
};

class WolkGatewayBuilder final
{
public:
    /**
     * @brief WolkBuilder Initiates wolkabout::Wolk builder
     * @param device Device for which wolkabout::WolkBuilder is instantiated
     */
    explicit WolkGatewayBuilder(Device device);

    /**
     * @brief Allows passing of URI to custom WolkAbout IoT platform instance
     * @param host Server URI
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& platformHost(const std::string& host);

    /**
     * @brief Allows passing of server certificate
     * @param trust store
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& platformTrustStore(const std::string& trustStore);

    /**
     * @brief Sets feed update handler
     * @param feedUpdateHandler Lambda that handles feed update requests. Will receive a map of readings grouped by the
     * time when the update happened. Key is a timestamp in milliseconds, and value is a vector of readings that changed
     * at that time.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& feedUpdateHandler(
      const std::function<void(std::string, const std::map<std::uint64_t, std::vector<Reading>>)>& feedUpdateHandler);

    /**
     * @brief Sets feed update handler
     * @param feedUpdateHandler Instance of wolkabout::FeedUpdateHandler
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& feedUpdateHandler(std::weak_ptr<connect::FeedUpdateHandler> feedUpdateHandler);

    /**
     * @brief Sets parameter handler
     * @param parameterHandlerLambda Lambda that handles parameters updates
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& parameterHandler(
      const std::function<void(std::string, std::vector<Parameter>)>& parameterHandlerLambda);

    /**
     * @brief Sets parameter handler
     * @param parameterHandler Instance of wolkabout::ParameterHandler
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& parameterHandler(std::weak_ptr<connect::ParameterHandler> parameterHandler);

    /**
     * @brief Sets underlying persistence for device services.
     * @param persistence std
     * @return
     */
    WolkGatewayBuilder& withPersistence(std::unique_ptr<Persistence> persistence);

    /**
     * @brief Sets underlying persistence mechanism to be used<br>
     *        Sample in-memory persistence is used as default
     * @param persistence std::unique_ptr to wolkabout::MessagePersistence implementation
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withMessagePersistence(std::unique_ptr<MessagePersistence> persistence);

    /**
     * @brief Sets the policy that will be used for caching device data.
     * @param policy The policy that will be used for storing device data.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& deviceStoragePolicy(DeviceStoragePolicy policy);

    /**
     * @brief Sets a custom existing device repository to be used by the Wolk object.
     * @param repository std::unique_ptr to gateway::ExistingDeviceRepository implementation
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withExistingDeviceRepository(std::unique_ptr<ExistingDevicesRepository> repository);

    /**
     * @brief withDataProtocol Defines which data protocol to use
     * @param Protocol unique_ptr to wolkabout::DataProtocol implementation
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withDataProtocol(std::unique_ptr<DataProtocol> protocol);

    /**
     * @brief withErrorProtocol Defines which error protocol to use
     * @param errorRetainTime The time defining how long will the error messages be retained. The default retain time is
     * 1s (1000ms).
     * @param protocol Unique_ptr to wolkabout::ErrorProtocol implementation (providing nullptr will still refer to
     * default one)
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withErrorProtocol(std::chrono::milliseconds errorRetainTime,
                                          std::unique_ptr<ErrorProtocol> protocol = nullptr);

    /**
     * @brief Sets the Wolk module to allow file management functionality.
     * @details This one is meant to enable the File Transfer, but not File URL Download.
     * @param fileDownloadLocation The folder location for file management.
     * @param maxPacketSize The maximum packet size for downloading chunks (in KBs).
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withFileTransfer(const std::string& fileDownloadLocation, std::uint64_t maxPacketSize = 268435);

    /**
     * @brief Sets the Wolk module to allow file management functionality.
     * @details This one is meant to enable File URL Download, but can enabled File Transfer too.
     * @param fileDownloadLocation The folder location for file management.
     * @param fileDownloader The implementation that will download the files.
     * @param transferEnabled Whether the File Transfer should be enabled too.
     * @param maxPacketSize The max packet size for downloading chunks (in MBs).
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withFileURLDownload(const std::string& fileDownloadLocation,
                                            std::shared_ptr<connect::FileDownloader> fileDownloader = nullptr,
                                            bool transferEnabled = false, std::uint64_t maxPacketSize = 268435);

    /**
     * @brief Sets the Wolk module file listener.
     * @details This object will receive information about newly obtained or removed files. It will be used with
     * `withFileListener` when the service gets created.
     * @param fileListener A pointer to the instance of the file listener.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withFileListener(const std::shared_ptr<connect::FileListener>& fileListener);

    /**
     * @brief Sets the Wolk module to allow firmware update functionality.
     * @details This one is meant for PUSH configuration, where the functionality is implemented using the
     * `FirmwareInstaller`. This object will received instructions from the platform of when to install new firmware.
     * @param firmwareInstaller The implementation of the FirmwareInstaller interface.
     * @param workingDirectory The directory where the session file will be kept.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withFirmwareUpdate(std::unique_ptr<connect::FirmwareInstaller> firmwareInstaller,
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
    WolkGatewayBuilder& withFirmwareUpdate(
      std::unique_ptr<connect::FirmwareParametersListener> firmwareParametersListener,
      const std::string& workingDirectory = "./");

    /**
     * @brief Sets the seconds the MQTT connection with the platform will be kept alive for.
     * @param keepAlive The amount of seconds the connection will be pinged.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& setMqttKeepAlive(std::uint16_t keepAlive);

    /**
     * @brief Sets the gateway to use the InternalData service that connects to a local MQTT message broker.
     * @param local The mqtt path that will be used to connect to a local MQTT broker.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withInternalDataService(const std::string& local = MESSAGE_BUS_HOST);

    /**
     * @brief Sets the gateway to use the DevicesService for communication with the platform.
     * @param platformProtocol The protocol which will be used for platform communication.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withPlatformRegistration(std::unique_ptr<RegistrationProtocol> platformProtocol = {});

    /**
     * @brief Sets the gateway to use the DevicesService for communication with the local broker - requires
     * .withInternalDataService to be invoked.
     * @param localProtocol The protocol which will be used for local communication.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withLocalRegistration(std::unique_ptr<GatewayRegistrationProtocol> localProtocol = {});

    /**
     * @brief Sets the data provider to engage an ExternalDataService.
     * @param dataProvider The object that will provide/receive data for devices.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withExternalDataService(DataProvider* dataProvider);

    /**
     * @brief Sets the gateway to use a platform status service, announcing the connection from the platform to the
     * local broker.
     * @param protocol The protocol that will be used to communicate. If left for default,
     * `wolkabout::WolkaboutGatewayPlatformStatusProtocol` will be initialized.
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkGatewayBuilder& withPlatformStatusService(std::unique_ptr<GatewayPlatformStatusProtocol> protocol = nullptr);

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
    std::string m_localMqttHost;

    // Here is the place for external entities capable of receiving Reading values.
    std::function<void(std::string, std::map<std::uint64_t, std::vector<Reading>>)> m_feedUpdateHandlerLambda;
    std::weak_ptr<connect::FeedUpdateHandler> m_feedUpdateHandler;

    // Here is the place for external entities capable of receiving Parameter values.
    std::function<void(std::string, std::vector<Parameter>)> m_parameterHandlerLambda;
    std::weak_ptr<connect::ParameterHandler> m_parameterHandler;

    // Place for the persistence objects
    std::unique_ptr<Persistence> m_persistence;
    std::unique_ptr<MessagePersistence> m_messagePersistence;

    // Place for the repository objects
    DeviceStoragePolicy m_deviceStoragePolicy;
    std::unique_ptr<ExistingDevicesRepository> m_existingDeviceRepository;

    // Here is the place for all the protocols that are being held
    std::unique_ptr<DataProtocol> m_dataProtocol;
    std::unique_ptr<ErrorProtocol> m_errorProtocol;
    std::chrono::milliseconds m_errorRetainTime;
    std::unique_ptr<FileManagementProtocol> m_fileManagementProtocol;
    std::unique_ptr<FirmwareUpdateProtocol> m_firmwareUpdateProtocol;
    std::unique_ptr<GatewayPlatformStatusProtocol> m_gatewayPlatformStatusProtocol;
    std::unique_ptr<GatewaySubdeviceProtocol> m_platformSubdeviceProtocol;
    std::unique_ptr<GatewaySubdeviceProtocol> m_localSubdeviceProtocol;
    std::unique_ptr<GatewayRegistrationProtocol> m_localRegistrationProtocol;
    std::unique_ptr<RegistrationProtocol> m_platformRegistrationProtocol;

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

    // Here is the data provider for the ExternalDataService
    DataProvider* m_dataProvider;

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
