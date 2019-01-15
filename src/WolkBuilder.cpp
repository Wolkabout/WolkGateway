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

#include "WolkBuilder.h"
#include "GatewayInboundDeviceMessageHandler.h"
#include "GatewayInboundPlatformMessageHandler.h"
#include "StatusMessageRouter.h"
#include "Wolk.h"
#include "connectivity/ConnectivityService.h"
#include "connectivity/mqtt/MqttConnectivityService.h"
#include "connectivity/mqtt/PahoMqttClient.h"
#include "model/Device.h"
#include "model/Message.h"
#include "persistence/inmemory/GatewayInMemoryPersistence.h"
#include "protocol/json/JsonGatewayDFUProtocol.h"
#include "protocol/json/JsonGatewayDataProtocol.h"
#include "protocol/json/JsonGatewayDeviceRegistrationProtocol.h"
#include "protocol/json/JsonGatewayStatusProtocol.h"
#include "protocol/json/JsonGatewayWolkDownloadProtocol.h"
#include "protocol/json/JsonProtocol.h"
#include "repository/ExistingDevicesRepository.h"
#include "repository/JsonFileExistingDevicesRepository.h"
#include "repository/SQLiteDeviceRepository.h"
#include "service/DeviceRegistrationService.h"
#include "service/DeviceStatusService.h"
#include "service/FileDownloadService.h"
#include "service/FirmwareUpdateService.h"
#include "service/KeepAliveService.h"
#include "service/PublishingService.h"

#include <stdexcept>

namespace wolkabout
{
WolkBuilder& WolkBuilder::platformHost(const std::string& host)
{
    m_platformHost = host;
    return *this;
}

WolkBuilder& WolkBuilder::platformTrustStore(const std::string& trustStore)
{
    m_platformTrustStore = trustStore;
    return *this;
}

WolkBuilder& WolkBuilder::gatewayHost(const std::string& host)
{
    m_gatewayHost = host;
    return *this;
}

WolkBuilder& WolkBuilder::actuationHandler(
  const std::function<void(const std::string&, const std::string&)>& actuationHandler)
{
    m_actuationHandlerLambda = actuationHandler;
    m_actuationHandler.reset();
    return *this;
}

WolkBuilder& WolkBuilder::actuationHandler(std::shared_ptr<ActuationHandler> actuationHandler)
{
    m_actuationHandler = actuationHandler;
    m_actuationHandlerLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::actuatorStatusProvider(
  const std::function<ActuatorStatus(const std::string&)>& actuatorStatusProvider)
{
    m_actuatorStatusProviderLambda = actuatorStatusProvider;
    m_actuatorStatusProvider.reset();
    return *this;
}

WolkBuilder& WolkBuilder::actuatorStatusProvider(std::shared_ptr<ActuatorStatusProvider> actuatorStatusProvider)
{
    m_actuatorStatusProvider = actuatorStatusProvider;
    m_actuatorStatusProviderLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::configurationHandler(
  std::function<void(const std::vector<ConfigurationItem>& configuration)> configurationHandler)
{
    m_configurationHandlerLambda = configurationHandler;
    m_configurationHandler.reset();
    return *this;
}

wolkabout::WolkBuilder& WolkBuilder::configurationHandler(std::shared_ptr<ConfigurationHandler> configurationHandler)
{
    m_configurationHandler = configurationHandler;
    m_configurationHandlerLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::configurationProvider(std::function<std::vector<ConfigurationItem>()> configurationProvider)
{
    m_configurationProviderLambda = configurationProvider;
    m_configurationProvider.reset();
    return *this;
}

wolkabout::WolkBuilder& WolkBuilder::configurationProvider(std::shared_ptr<ConfigurationProvider> configurationProvider)
{
    m_configurationProvider = configurationProvider;
    m_configurationProviderLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::withDataProtocol(std::shared_ptr<GatewayDataProtocol> protocol)
{
    m_dataProtocol = protocol;
    return *this;
}

WolkBuilder& WolkBuilder::withFirmwareUpdate(const std::string& firmwareVersion,
                                             std::shared_ptr<FirmwareInstaller> installer,
                                             const std::string& firmwareDownloadDirectory,
                                             uint_fast64_t maxFirmwareFileSize,
                                             std::uint_fast64_t maxFirmwareFileChunkSize,
                                             std::shared_ptr<UrlFileDownloader> urlDownloader)
{
    m_firmwareVersion = firmwareVersion;
    m_firmwareDownloadDirectory = firmwareDownloadDirectory;
    m_maxFirmwareFileSize = maxFirmwareFileSize;
    m_maxFirmwareFileChunkSize = maxFirmwareFileChunkSize;
    m_firmwareInstaller = installer;
    m_urlFileDownloader = urlDownloader;
    return *this;
}

WolkBuilder& WolkBuilder::withoutKeepAlive()
{
    m_keepAliveEnabled = false;
    return *this;
}

std::unique_ptr<Wolk> WolkBuilder::build()
{
    if (m_device.getKey().empty())
    {
        throw std::logic_error("No device key present.");
    }

    if (!m_dataProtocol)
    {
        throw std::logic_error("No protocol defined.");
    }

    if (m_device.getActuatorReferences().size() != 0)
    {
        if (m_actuationHandler == nullptr && m_actuationHandlerLambda == nullptr)
        {
            throw std::logic_error("Actuation handler not set.");
        }

        if (m_actuatorStatusProvider == nullptr && m_actuatorStatusProviderLambda == nullptr)
        {
            throw std::logic_error("Actuator status provider not set.");
        }
    }

    if (!m_configurationHandlerLambda != !m_configurationProviderLambda)
    {
        throw std::logic_error("Both ConfigurationProvider and ConfigurationHandler must be set.");
    }

    if (!m_configurationHandler != !m_configurationProvider)
    {
        throw std::logic_error("Both ConfigurationProvider and ConfigurationHandler must be set.");
    }

    auto wolk = std::unique_ptr<Wolk>(new Wolk(m_device));

    wolk->m_statusProtocol = std::unique_ptr<GatewayStatusProtocol>(new JsonGatewayStatusProtocol());
    wolk->m_registrationProtocol =
      std::unique_ptr<GatewayDeviceRegistrationProtocol>(new JsonGatewayDeviceRegistrationProtocol());
    wolk->m_fileDownloadProtocol = std::unique_ptr<GatewayFileDownloadProtocol>(new JsonGatewayWolkDownloadProtocol());
    wolk->m_firmwareUpdateProtocol = std::unique_ptr<GatewayFirmwareUpdateProtocol>(new JsonGatewayDFUProtocol());

    // Setup device repository
    wolk->m_deviceRepository.reset(new SQLiteDeviceRepository());

    // Setup existing devices repository
    wolk->m_existingDevicesRepository.reset(new JsonFileExistingDevicesRepository());

    // Setup connectivity services
    wolk->m_platformConnectivityService.reset(new MqttConnectivityService(std::make_shared<PahoMqttClient>(),
                                                                          m_device.getKey(), m_device.getPassword(),
                                                                          m_platformHost, m_platformTrustStore));
    wolk->m_platformConnectivityService->setUncontrolledDisonnectMessage(
      wolk->m_statusProtocol->makeLastWillMessage(m_device.getKey()));

    const std::string localMqttClientId = std::string("Gateway-").append(m_device.getKey());
    wolk->m_deviceConnectivityService.reset(new MqttConnectivityService(
      std::make_shared<PahoMqttClient>(), m_device.getKey(), m_device.getPassword(), m_gatewayHost, localMqttClientId));

    wolk->m_platformPublisher.reset(new PublishingService(
      *wolk->m_platformConnectivityService, std::unique_ptr<GatewayPersistence>(new GatewayInMemoryPersistence())));
    wolk->m_devicePublisher.reset(new PublishingService(
      *wolk->m_deviceConnectivityService, std::unique_ptr<GatewayPersistence>(new GatewayInMemoryPersistence())));

    wolk->m_inboundPlatformMessageHandler.reset(new GatewayInboundPlatformMessageHandler(m_device.getKey()));
    wolk->m_inboundDeviceMessageHandler.reset(new GatewayInboundDeviceMessageHandler());

    wolk->m_platformConnectivityManager = std::make_shared<Wolk::ConnectivityFacade<InboundPlatformMessageHandler>>(
      *wolk->m_inboundPlatformMessageHandler, [&] {
          wolk->notifyPlatformDisonnected();
          wolk->connectToPlatform();
      });

    wolk->m_deviceConnectivityManager = std::make_shared<Wolk::ConnectivityFacade<InboundDeviceMessageHandler>>(
      *wolk->m_inboundDeviceMessageHandler, [&] {
          wolk->notifyDevicesDisonnected();
          wolk->connectToDevices();
      });

    wolk->m_platformConnectivityService->setListener(wolk->m_platformConnectivityManager);
    wolk->m_deviceConnectivityService->setListener(wolk->m_deviceConnectivityManager);

    wolk->m_actuationHandlerLambda = m_actuationHandlerLambda;
    wolk->m_actuationHandler = m_actuationHandler;

    wolk->m_actuatorStatusProviderLambda = m_actuatorStatusProviderLambda;
    wolk->m_actuatorStatusProvider = m_actuatorStatusProvider;

    wolk->m_configurationHandlerLambda = m_configurationHandlerLambda;
    wolk->m_configurationHandler = m_configurationHandler;

    wolk->m_configurationProviderLambda = m_configurationProviderLambda;
    wolk->m_configurationProvider = m_configurationProvider;

    // Setup registration service
    wolk->m_deviceRegistrationService = std::make_shared<DeviceRegistrationService>(
      m_device.getKey(), *wolk->m_registrationProtocol, *wolk->m_deviceRepository, *wolk->m_platformPublisher,
      *wolk->m_devicePublisher);

    wolk->m_inboundDeviceMessageHandler->addListener(wolk->m_deviceRegistrationService);
    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_deviceRegistrationService);

    wolk->m_deviceRegistrationService->onDeviceRegistered([&](const std::string& deviceKey, bool isGateway) {
        if (isGateway)
        {
            wolk->m_keepAliveService->sendPingMessage();
        }
        else
        {
            wolk->m_deviceStatusService->sendLastKnownStatusForDevice(deviceKey);
            wolk->m_existingDevicesRepository->addDeviceKey(deviceKey);
        }
    });

    // Setup device status and keep alive service
    wolk->m_deviceStatusService = std::make_shared<DeviceStatusService>(
      m_device.getKey(), *wolk->m_statusProtocol, *wolk->m_deviceRepository, *wolk->m_platformPublisher,
      *wolk->m_devicePublisher, Wolk::KEEP_ALIVE_INTERVAL);

    if (m_keepAliveEnabled)
    {
        wolk->m_keepAliveService = std::make_shared<KeepAliveService>(
          m_device.getKey(), *wolk->m_statusProtocol, *wolk->m_platformPublisher, Wolk::KEEP_ALIVE_INTERVAL);
    }

    wolk->m_statusMessageRouter = std::make_shared<StatusMessageRouter>(
      *wolk->m_statusProtocol, wolk->m_deviceStatusService.get(), wolk->m_deviceStatusService.get(),
      wolk->m_deviceStatusService.get(), wolk->m_keepAliveService.get());

    wolk->m_inboundDeviceMessageHandler->addListener(wolk->m_statusMessageRouter);
    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_statusMessageRouter);

    // setup gateway data service if gateway manifest is not empty
    const auto gwManifest = m_device.getManifest();
    if (!gwManifest.getSensors().empty() || !gwManifest.getActuators().empty() || !gwManifest.getAlarms().empty() ||
        !gwManifest.getConfigurations().empty())
    {
        auto dataService = std::make_shared<DataService>(m_device.getKey(), *m_dataProtocol, *wolk->m_deviceRepository,
                                                         *wolk->m_platformPublisher, *wolk->m_devicePublisher);

        wolk->m_gatewayPersistence.reset(new InMemoryPersistence());
        wolk->m_gatewayDataProtocol.reset(new JsonProtocol());
        wolk->m_gatewayDataService.reset(new GatewayDataService(
          m_device.getKey(), *wolk->m_gatewayDataProtocol, *wolk->m_gatewayPersistence, *dataService,
          [&](const std::string& reference, const std::string& value) {
              wolk->handleActuatorSetCommand(reference, value);
          },
          [&](const std::string& reference) { wolk->handleActuatorGetCommand(reference); },
          [&](const ConfigurationSetCommand& command) { wolk->handleConfigurationSetCommand(command); },
          [&]() { wolk->handleConfigurationGetCommand(); }));

        dataService->setGatewayMessageListener(wolk->m_gatewayDataService.get());

        // Setup data service
        wolk->registerDataProtocol(m_dataProtocol, dataService);
    }
    else
    {
        // Setup data service
        wolk->registerDataProtocol(m_dataProtocol);
    }

    // setup file download service
    wolk->m_fileDownloadService =
      std::make_shared<FileDownloadService>(m_device.getKey(), *wolk->m_fileDownloadProtocol, m_maxFirmwareFileSize,
                                            m_maxFirmwareFileChunkSize, *wolk->m_platformPublisher);
    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_fileDownloadService);

    // setup firmware update service
    wolk->m_firmwareUpdateService = std::make_shared<FirmwareUpdateService>(
      m_device.getKey(), *wolk->m_firmwareUpdateProtocol, *wolk->m_platformPublisher, *wolk->m_devicePublisher,
      *wolk->m_fileDownloadService, m_firmwareDownloadDirectory, m_firmwareInstaller, m_firmwareVersion,
      m_urlFileDownloader);
    wolk->m_inboundDeviceMessageHandler->addListener(wolk->m_firmwareUpdateService);
    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_firmwareUpdateService);

    return wolk;
}

wolkabout::WolkBuilder::operator std::unique_ptr<Wolk>()
{
    return build();
}

WolkBuilder::WolkBuilder(Device device)
: m_platformHost{WOLK_DEMO_HOST}, m_gatewayHost{MESSAGE_BUS_HOST}, m_device{std::move(device)}, m_keepAliveEnabled{true}
{
}
}    // namespace wolkabout
