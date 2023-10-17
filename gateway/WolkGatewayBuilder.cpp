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

#include "gateway/WolkGatewayBuilder.h"

#include "core/connectivity/InboundPlatformMessageHandler.h"
#include "core/connectivity/OutboundRetryMessageHandler.h"
#include "core/connectivity/mqtt/MqttConnectivityService.h"
#include "core/connectivity/mqtt/PahoMqttClient.h"
#include "core/persistence/inmemory/InMemoryMessagePersistence.h"
#include "core/persistence/inmemory/InMemoryPersistence.h"
#include "core/protocol/wolkabout/WolkaboutDataProtocol.h"
#include "core/protocol/wolkabout/WolkaboutErrorProtocol.h"
#include "core/protocol/wolkabout/WolkaboutFileManagementProtocol.h"
#include "core/protocol/wolkabout/WolkaboutFirmwareUpdateProtocol.h"
#include "core/protocol/wolkabout/WolkaboutGatewayPlatformStatusProtocol.h"
#include "core/protocol/wolkabout/WolkaboutGatewayRegistrationProtocol.h"
#include "core/protocol/wolkabout/WolkaboutGatewaySubdeviceProtocol.h"
#include "core/protocol/wolkabout/WolkaboutRegistrationProtocol.h"
#include "gateway/WolkGateway.h"
#include "gateway/connectivity/GatewayMessageRouter.h"
#include "gateway/repository/device/InMemoryDeviceRepository.h"
#include "gateway/repository/device/SQLiteDeviceRepository.h"
#include "gateway/repository/existing_device/JsonFileExistingDevicesRepository.h"
#include "gateway/service/devices/DevicesService.h"
#include "gateway/service/external_data/ExternalDataService.h"
#include "gateway/service/internal_data/InternalDataService.h"
#include "gateway/service/platform_status/GatewayPlatformStatusService.h"

#include <memory>
#include <stdexcept>

using namespace wolkabout::legacy;

namespace wolkabout::gateway
{
WolkGatewayBuilder::WolkGatewayBuilder(Device device)
: m_device{std::move(device)}
, m_platformHost{WOLK_HOST}
, m_platformMqttKeepAliveSec{60}
, m_persistence{new InMemoryPersistence}
, m_messagePersistence{new InMemoryMessagePersistence}
, m_deviceStoragePolicy{DeviceStoragePolicy::FULL}
, m_existingDeviceRepository{new JsonFileExistingDevicesRepository}
, m_dataProtocol{new WolkaboutDataProtocol}
, m_errorProtocol{new WolkaboutErrorProtocol}
, m_errorRetainTime{1}
, m_platformSubdeviceProtocol{new WolkaboutGatewaySubdeviceProtocol}
, m_localSubdeviceProtocol{new WolkaboutGatewaySubdeviceProtocol(false)}
, m_platformRegistrationProtocol{new WolkaboutRegistrationProtocol}
, m_fileTransferEnabled{false}
, m_fileTransferUrlEnabled{false}
, m_maxPacketSize{MAX_PACKET_SIZE}
, m_firmwareParametersListener{nullptr}
, m_dataProvider{nullptr}
{
}

WolkGatewayBuilder& WolkGatewayBuilder::platformHost(const std::string& host)
{
    m_platformHost = host;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::platformTrustStore(const std::string& trustStore)
{
    m_platformTrustStore = trustStore;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::feedUpdateHandler(
  const std::function<void(std::string, const std::map<std::uint64_t, std::vector<Reading>>)>& feedUpdateHandler)
{
    m_feedUpdateHandlerLambda = feedUpdateHandler;
    m_feedUpdateHandler.reset();
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::feedUpdateHandler(std::weak_ptr<connect::FeedUpdateHandler> feedUpdateHandler)
{
    m_feedUpdateHandler = std::move(feedUpdateHandler);
    m_feedUpdateHandlerLambda = nullptr;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::parameterHandler(
  const std::function<void(std::string, std::vector<Parameter>)>& parameterHandlerLambda)
{
    m_parameterHandlerLambda = parameterHandlerLambda;
    m_parameterHandler.reset();
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::parameterHandler(std::weak_ptr<connect::ParameterHandler> parameterHandler)
{
    m_parameterHandler = std::move(parameterHandler);
    m_parameterHandlerLambda = nullptr;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withPersistence(std::unique_ptr<Persistence> persistence)
{
    m_persistence = std::move(persistence);
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withMessagePersistence(std::unique_ptr<MessagePersistence> persistence)
{
    m_messagePersistence = std::move(persistence);
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::deviceStoragePolicy(DeviceStoragePolicy policy)
{
    m_deviceStoragePolicy = policy;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withExistingDeviceRepository(
  std::unique_ptr<ExistingDevicesRepository> repository)
{
    m_existingDeviceRepository = std::move(repository);
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withDataProtocol(std::unique_ptr<DataProtocol> protocol)
{
    m_dataProtocol = std::move(protocol);
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withErrorProtocol(std::chrono::milliseconds errorRetainTime,
                                                          std::unique_ptr<ErrorProtocol> protocol)
{
    m_errorRetainTime = errorRetainTime;
    if (protocol != nullptr)
        m_errorProtocol = std::move(protocol);
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withFileTransfer(const std::string& fileDownloadLocation,
                                                         std::uint64_t maxPacketSize)
{
    if (m_firmwareUpdateProtocol == nullptr)
        m_fileManagementProtocol =
          std::unique_ptr<WolkaboutFileManagementProtocol>(new wolkabout::WolkaboutFileManagementProtocol);
    m_fileDownloadDirectory = fileDownloadLocation;
    m_fileTransferEnabled = true;
    m_fileTransferUrlEnabled = false;
    m_fileDownloader = nullptr;
    m_maxPacketSize = maxPacketSize;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withFileURLDownload(const std::string& fileDownloadLocation,
                                                            std::shared_ptr<connect::FileDownloader> fileDownloader,
                                                            bool transferEnabled, std::uint64_t maxPacketSize)
{
    if (m_firmwareUpdateProtocol == nullptr)
        m_fileManagementProtocol =
          std::unique_ptr<WolkaboutFileManagementProtocol>(new wolkabout::WolkaboutFileManagementProtocol);
    m_fileDownloadDirectory = fileDownloadLocation;
    m_fileTransferEnabled = transferEnabled;
    m_fileTransferUrlEnabled = true;
    m_fileDownloader = std::move(fileDownloader);
    m_maxPacketSize = maxPacketSize;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withFileListener(const std::shared_ptr<connect::FileListener>& fileListener)
{
    m_fileListener = fileListener;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withFirmwareUpdate(
  std::unique_ptr<connect::FirmwareInstaller> firmwareInstaller, const std::string& workingDirectory)
{
    if (m_firmwareUpdateProtocol == nullptr)
        m_firmwareUpdateProtocol =
          std::unique_ptr<WolkaboutFirmwareUpdateProtocol>(new wolkabout::WolkaboutFirmwareUpdateProtocol);
    m_firmwareParametersListener = nullptr;
    m_firmwareInstaller = std::move(firmwareInstaller);
    m_workingDirectory = workingDirectory;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withFirmwareUpdate(
  std::unique_ptr<connect::FirmwareParametersListener> firmwareParametersListener, const std::string& workingDirectory)
{
    if (m_firmwareUpdateProtocol == nullptr)
        m_firmwareUpdateProtocol =
          std::unique_ptr<WolkaboutFirmwareUpdateProtocol>(new wolkabout::WolkaboutFirmwareUpdateProtocol);
    m_firmwareInstaller = nullptr;
    m_firmwareParametersListener = std::move(firmwareParametersListener);
    m_workingDirectory = workingDirectory;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::setMqttKeepAlive(std::uint16_t keepAlive)
{
    m_platformMqttKeepAliveSec = keepAlive;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withInternalDataService(const std::string& local)
{
    m_localMqttHost = local;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withPlatformRegistration(std::unique_ptr<RegistrationProtocol> platformProtocol)
{
    if (platformProtocol == nullptr)
        platformProtocol = std::unique_ptr<WolkaboutRegistrationProtocol>{new WolkaboutRegistrationProtocol};
    m_platformRegistrationProtocol = std::move(platformProtocol);
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withLocalRegistration(
  std::unique_ptr<GatewayRegistrationProtocol> localProtocol)
{
    if (localProtocol == nullptr)
        localProtocol = std::unique_ptr<WolkaboutGatewayRegistrationProtocol>{new WolkaboutGatewayRegistrationProtocol};
    m_localRegistrationProtocol = std::move(localProtocol);
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withExternalDataService(DataProvider* dataProvider)
{
    m_dataProvider = dataProvider;
    return *this;
}

WolkGatewayBuilder& WolkGatewayBuilder::withPlatformStatusService(
  std::unique_ptr<GatewayPlatformStatusProtocol> protocol)
{
    if (protocol == nullptr)
        protocol = std::unique_ptr<WolkaboutGatewayPlatformStatusProtocol>{new WolkaboutGatewayPlatformStatusProtocol};
    m_gatewayPlatformStatusProtocol = std::move(protocol);
    return *this;
}

std::unique_ptr<WolkGateway> WolkGatewayBuilder::build()
{
    // Right away check a bunch of the parameters
    if (m_device.getKey().empty())
        throw std::logic_error("No device key present.");

    // Create the instance of the Wolk based on the data provider source.
    auto wolk = std::unique_ptr<WolkGateway>{new WolkGateway{m_device}};
    auto wolkRaw = wolk.get();

    // Move the persistence objects
    wolk->m_persistence = std::move(m_persistence);
    wolk->m_messagePersistence = std::move(m_messagePersistence);

    // Move the repository objects
    if (m_deviceStoragePolicy == DeviceStoragePolicy::PERSISTENT || m_deviceStoragePolicy == DeviceStoragePolicy::FULL)
    {
        wolk->m_persistentDeviceRepository = std::make_shared<SQLiteDeviceRepository>();
        wolk->m_existingDevicesRepository = std::make_shared<JsonFileExistingDevicesRepository>();
    }
    if (m_deviceStoragePolicy == DeviceStoragePolicy::CACHED || m_deviceStoragePolicy == DeviceStoragePolicy::FULL)
    {
        wolk->m_cacheDeviceRepository = std::make_shared<InMemoryDeviceRepository>(wolk->m_persistentDeviceRepository);
    }
    wolk->m_existingDevicesRepository = std::move(m_existingDeviceRepository);

    // Create the platform connection
    auto mqttClient = std::make_shared<PahoMqttClient>(m_platformMqttKeepAliveSec);
    wolk->m_connectivityService = std::unique_ptr<MqttConnectivityService>{new MqttConnectivityService{
      std::move(mqttClient), m_device.getKey(), m_device.getPassword(), m_platformHost, m_platformTrustStore,
      ByteUtils::toUUIDString(ByteUtils::generateRandomBytes(ByteUtils::UUID_VECTOR_SIZE)),
      wolk->m_messagePersistence}};
    wolk->m_outboundMessageHandler = dynamic_cast<MqttConnectivityService*>(wolk->m_connectivityService.get());
    wolk->m_outboundRetryMessageHandler =
      std::unique_ptr<OutboundRetryMessageHandler>{new OutboundRetryMessageHandler{*wolk->m_outboundMessageHandler}};

    // Set up the connection links
    wolk->m_inboundMessageHandler =
      std::make_shared<InboundPlatformMessageHandler>(std::vector<std::string>{m_device.getKey()});
    wolk->m_connectivityService->onConnectionLost([wolkRaw] {
        wolkRaw->notifyPlatformDisconnected();
        wolkRaw->connectPlatform(true);
    });
    wolk->m_connectivityService->setListner(wolk->m_inboundMessageHandler);

    // Set up the gateway message router
    wolk->m_platformSubdeviceProtocol = std::move(m_platformSubdeviceProtocol);
    wolk->m_localSubdeviceProtocol = std::move(m_localSubdeviceProtocol);
    wolk->m_gatewayMessageRouter = std::make_shared<GatewayMessageRouter>(*wolk->m_platformSubdeviceProtocol);
    wolk->m_inboundMessageHandler->addListener(wolk->m_gatewayMessageRouter);

    // Set up the device services
    wolk->m_dataProtocol = std::move(m_dataProtocol);
    wolk->m_errorProtocol = std::move(m_errorProtocol);
    wolk->m_feedUpdateHandlerLambda = m_feedUpdateHandlerLambda;
    wolk->m_feedUpdateHandler = m_feedUpdateHandler;
    wolk->m_parameterLambda = m_parameterHandlerLambda;
    wolk->m_parameterHandler = m_parameterHandler;
    wolk->m_dataService = std::make_shared<connect::DataService>(
      *wolk->m_dataProtocol, *wolk->m_persistence, *wolk->m_connectivityService, *wolk->m_outboundRetryMessageHandler,
      [wolkRaw](const std::string& deviceKey, const std::map<std::uint64_t, std::vector<Reading>>& readings) {
          wolkRaw->handleFeedUpdateCommand(deviceKey, readings);
      },
      [wolkRaw](const std::string& deviceKey, const std::vector<Parameter>& parameters) {
          wolkRaw->handleParameterCommand(deviceKey, parameters);
      },
      [](const std::string& deviceKey, const std::vector<std::string>& feeds,
         const std::vector<std::string>& attributes) {
          LOG(DEBUG) << "Received details for device '" << deviceKey << "':";
          LOG(DEBUG) << "Feeds: ";
          for (const auto& feed : feeds)
              LOG(DEBUG) << "\t" << feed;
          LOG(DEBUG) << "Attributes: ";
          for (const auto& attribute : attributes)
              LOG(DEBUG) << "\t" << attribute;
      });
    wolk->m_errorService = std::make_shared<connect::ErrorService>(*wolk->m_errorProtocol, m_errorRetainTime);
    wolk->m_inboundMessageHandler->addListener(wolk->m_dataService);
    wolk->m_inboundMessageHandler->addListener(wolk->m_errorService);
    wolk->m_errorService->start();

    // Update the parameters
    wolk->m_dataService->updateParameter(
      m_device.getKey(), {ParameterName::FILE_TRANSFER_PLATFORM_ENABLED, m_fileTransferEnabled ? "true" : "false"});
    wolk->m_dataService->updateParameter(
      m_device.getKey(), {ParameterName::FILE_TRANSFER_URL_ENABLED, m_fileTransferUrlEnabled ? "true" : "false"});

    // Check if the file management should be engaged
    if (m_fileManagementProtocol != nullptr)
    {
        // Create the File Management service
        wolk->m_fileManagementProtocol = std::move(m_fileManagementProtocol);
        wolk->m_fileManagementService = std::make_shared<connect::FileManagementService>(
          *wolk->m_connectivityService, *wolk->m_dataService, *wolk->m_fileManagementProtocol, m_fileDownloadDirectory,
          m_fileTransferEnabled, m_fileTransferUrlEnabled, std::move(m_fileDownloader), std::move(m_fileListener));

        // Trigger the on build and add the listener for MQTT messages
        wolk->m_fileManagementService->createFolder();
        wolk->m_inboundMessageHandler->addListener(wolk->m_fileManagementService);
    }

    // Set the parameters about the FirmwareUpdate
    wolk->m_dataService->updateParameter(m_device.getKey(), {ParameterName::FIRMWARE_UPDATE_ENABLED,
                                                             m_firmwareUpdateProtocol != nullptr ? "true" : "false"});
    auto firmwareVersion = std::string{};
    if (m_firmwareInstaller != nullptr)
        firmwareVersion = m_firmwareInstaller->getFirmwareVersion(m_device.getKey());
    else if (m_firmwareParametersListener != nullptr)
        firmwareVersion = m_firmwareParametersListener->getFirmwareVersion();
    wolk->m_dataService->updateParameter(m_device.getKey(), {ParameterName::FIRMWARE_VERSION, firmwareVersion});

    // Check if the firmware update should be engaged
    if (m_firmwareUpdateProtocol != nullptr)
    {
        // Create the Firmware Update service
        wolk->m_firmwareUpdateProtocol = std::move(m_firmwareUpdateProtocol);

        // Build based on the module we have
        if (m_firmwareInstaller != nullptr)
        {
            wolk->m_firmwareUpdateService = std::make_shared<connect::FirmwareUpdateService>(
              *wolk->m_connectivityService, *wolk->m_dataService, wolk->m_fileManagementService,
              std::move(m_firmwareInstaller), *wolk->m_firmwareUpdateProtocol, m_workingDirectory);
        }
        else if (m_firmwareParametersListener != nullptr)
        {
            wolk->m_firmwareUpdateService = std::make_shared<connect::FirmwareUpdateService>(
              *wolk->m_connectivityService, *wolk->m_dataService, wolk->m_fileManagementService,
              std::move(m_firmwareParametersListener), *wolk->m_firmwareUpdateProtocol, m_workingDirectory);
        }

        // And set it all up
        wolk->m_firmwareUpdateService->loadState(m_device.getKey());
        wolk->m_inboundMessageHandler->addListener(wolk->m_firmwareUpdateService);
    }

    // Check if the internal data service needs to be set up
    if (!m_localMqttHost.empty())
    {
        // Create the local connectivity services
        auto localMqttClient = std::make_shared<PahoMqttClient>();
        auto localConnectivityService = std::make_shared<MqttConnectivityService>(
          std::move(localMqttClient), "", "", m_localMqttHost, "", m_device.getKey());
        wolk->m_localConnectivityService = localConnectivityService;
        wolk->m_localInboundMessageHandler =
          std::make_shared<InboundPlatformMessageHandler>(std::vector<std::string>{"+"});
        wolk->m_localConnectivityService->setListner(wolk->m_localInboundMessageHandler);
        wolk->m_localOutboundMessageHandler = localConnectivityService;

        // Set up the internal data service
        wolk->m_internalDataService =
          std::make_shared<InternalDataService>(m_device.getKey(), *wolk->m_outboundMessageHandler,
                                                *wolk->m_localOutboundMessageHandler, *wolk->m_localSubdeviceProtocol);
        wolk->m_gatewayMessageRouter->addListener("InternalDataService", wolk->m_internalDataService);
        wolk->m_localInboundMessageHandler->addListener(wolk->m_internalDataService);
    }

    // Set up the external data service if it needs to be set up
    if (m_dataProvider != nullptr)
    {
        // Create the external data service
        wolk->m_externalDataService = std::make_shared<ExternalDataService>(
          m_device.getKey(), *wolk->m_platformSubdeviceProtocol, *wolk->m_dataProtocol, *wolk->m_outboundMessageHandler,
          *m_dataProvider);
        m_dataProvider->setDataHandler(wolk->m_externalDataService.get(), m_device.getKey());
        wolk->m_gatewayMessageRouter->addListener("ExternalDataService", wolk->m_externalDataService);
    }

    // Set up the subdevice management service
    if (m_platformRegistrationProtocol != nullptr)
    {
        // Create the registration service
        wolk->m_platformRegistrationProtocol = std::move(m_platformRegistrationProtocol);
        wolk->m_localRegistrationProtocol = std::move(m_localRegistrationProtocol);
        wolk->m_subdeviceManagementService = std::make_shared<DevicesService>(
          m_device.getKey(), *wolk->m_platformRegistrationProtocol, *wolk->m_outboundMessageHandler,
          *wolk->m_outboundRetryMessageHandler, wolk->m_localRegistrationProtocol, wolk->m_localOutboundMessageHandler,
          wolk->m_cacheDeviceRepository != nullptr ? wolk->m_cacheDeviceRepository : wolk->m_persistentDeviceRepository,
          wolk->m_existingDevicesRepository);
        wolk->m_gatewayMessageRouter->addListener("SubdeviceManagement", wolk->m_subdeviceManagementService);
        if (wolk->m_localConnectivityService != nullptr && wolk->m_localRegistrationProtocol != nullptr)
            wolk->m_localInboundMessageHandler->addListener(wolk->m_subdeviceManagementService);
    }

    // Set up the platform status service
    if (wolk->m_localConnectivityService != nullptr && m_gatewayPlatformStatusProtocol != nullptr)
    {
        // Create the platform status service
        wolk->m_gatewayPlatformStatusProtocol = std::move(m_gatewayPlatformStatusProtocol);
        wolk->m_gatewayPlatformStatusService = std::make_shared<GatewayPlatformStatusService>(
          *wolk->m_localConnectivityService, *wolk->m_gatewayPlatformStatusProtocol, m_device.getKey());
    }

    return wolk;
}
} // namespace wolkabout::gateway

