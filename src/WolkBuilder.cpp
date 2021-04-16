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

#include "WolkBuilder.h"

#include "GatewayInboundDeviceMessageHandler.h"
#include "GatewayInboundPlatformMessageHandler.h"
#include "RegistrationMessageRouter.h"
#include "StatusMessageRouter.h"
#include "Wolk.h"
#include "WolkDefault.h"
#include "WolkExternal.h"
#include "core/connectivity/ConnectivityService.h"
#include "core/connectivity/mqtt/MqttConnectivityService.h"
#include "core/connectivity/mqtt/PahoMqttClient.h"
#include "core/model/Message.h"
#include "core/persistence/InMemoryPersistence.h"
#include "core/protocol/json/JsonDFUProtocol.h"
#include "core/protocol/json/JsonDownloadProtocol.h"
#include "core/protocol/json/JsonProtocol.h"
#include "core/protocol/json/JsonRegistrationProtocol.h"
#include "core/protocol/json/JsonStatusProtocol.h"
#include "model/GatewayDevice.h"
#include "persistence/inmemory/GatewayInMemoryPersistence.h"
#include "protocol/json/JsonGatewayDFUProtocol.h"
#include "protocol/json/JsonGatewayDataProtocol.h"
#include "protocol/json/JsonGatewayStatusProtocol.h"
#include "protocol/json/JsonGatewaySubdeviceRegistrationProtocol.h"
#include "repository/ExistingDevicesRepository.h"
#include "repository/JsonFileExistingDevicesRepository.h"
#include "repository/SQLiteDeviceRepository.h"
#include "repository/SQLiteFileRepository.h"
#include "service/FileDownloadService.h"
#include "service/FirmwareUpdateService.h"
#include "service/GatewayUpdateService.h"
#include "service/KeepAliveService.h"
#include "service/PublishingService.h"
#include "service/SubdeviceRegistrationService.h"
#include "service/data/ExternalDataService.h"
#include "service/data/GatewayDataService.h"
#include "service/data/InternalDataService.h"
#include "service/status/DeviceStatusService.h"
#include "service/status/ExternalDeviceStatusService.h"
#include "service/status/InternalDeviceStatusService.h"

#include <memory>
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

WolkBuilder& WolkBuilder::actuationHandler(std::function<void(const std::string&, const std::string&)> actuationHandler)
{
    m_actuationHandlerLambda = std::move(actuationHandler);
    m_actuationHandler.reset();
    return *this;
}

WolkBuilder& WolkBuilder::actuationHandler(std::shared_ptr<ActuationHandler> actuationHandler)
{
    m_actuationHandler = std::move(actuationHandler);
    m_actuationHandlerLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::actuatorStatusProvider(
  std::function<ActuatorStatus(const std::string&)> actuatorStatusProvider)
{
    m_actuatorStatusProviderLambda = std::move(actuatorStatusProvider);
    m_actuatorStatusProvider.reset();
    return *this;
}

WolkBuilder& WolkBuilder::actuatorStatusProvider(std::shared_ptr<ActuatorStatusProvider> actuatorStatusProvider)
{
    m_actuatorStatusProvider = std::move(actuatorStatusProvider);
    m_actuatorStatusProviderLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::configurationHandler(
  std::function<void(const std::vector<ConfigurationItem>& configuration)> configurationHandler)
{
    m_configurationHandlerLambda = std::move(configurationHandler);
    m_configurationHandler.reset();
    return *this;
}

wolkabout::WolkBuilder& WolkBuilder::configurationHandler(std::shared_ptr<ConfigurationHandler> configurationHandler)
{
    m_configurationHandler = std::move(configurationHandler);
    m_configurationHandlerLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::configurationProvider(std::function<std::vector<ConfigurationItem>()> configurationProvider)
{
    m_configurationProviderLambda = std::move(configurationProvider);
    m_configurationProvider.reset();
    return *this;
}

wolkabout::WolkBuilder& WolkBuilder::configurationProvider(std::shared_ptr<ConfigurationProvider> configurationProvider)
{
    m_configurationProvider = std::move(configurationProvider);
    m_configurationProviderLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::withFirmwareUpdate(const std::string& firmwareVersion,
                                             std::shared_ptr<FirmwareInstaller> installer)
{
    m_firmwareVersion = firmwareVersion;
    m_firmwareInstaller = std::move(installer);
    return *this;
}

WolkBuilder& WolkBuilder::withUrlFileDownload(std::shared_ptr<UrlFileDownloader> urlDownloader)
{
    m_urlFileDownloader = std::move(urlDownloader);
    return *this;
}

WolkBuilder& WolkBuilder::fileDownloadDirectory(const std::string& path)
{
    m_fileDownloadDirectory = path;
    return *this;
}

WolkBuilder& WolkBuilder::withExternalDataProvider(DataProvider* provider)
{
    m_externalDataProvider = provider;
    return *this;
}

WolkBuilder& WolkBuilder::withProtocol(std::unique_ptr<DataProtocol> dataProtocol, std::unique_ptr<StatusProtocol> statusProtocol)
{
    m_dataProtocol = std::move(dataProtocol);
    m_statusProtocol = std::move(statusProtocol);
    return *this;
}

std::unique_ptr<Wolk> WolkBuilder::build()
{
    if (m_device.getKey().empty())
    {
        throw std::logic_error("No device key present.");
    }

    if (!m_device.getActuatorReferences().empty())
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

    if (!m_device.getSubdeviceManagement())
    {
        throw std::logic_error("Subdevice management must be specified");
    }

    if (m_device.getFirmwareUpdate() && m_device.getFirmwareUpdate().value() &&
        m_device.getTemplate().getFirmwareUpdateType().empty())
    {
        throw std::logic_error("Firmware installer must be provided when firmware update is enabled");
    }

    if (m_device.getUrlDownload() && m_device.getUrlDownload().value() && !m_urlFileDownloader)
    {
        throw std::logic_error("Url downloader must be provided when url download is enabled");
    }

    if ((m_dataProtocol == nullptr && m_statusProtocol != nullptr) ||
        (m_dataProtocol != nullptr && m_statusProtocol == nullptr))
    {
        throw std::logic_error("Both data and status protocols must be set");
    }

    auto wolk = [&] {
        if (m_externalDataProvider)
        {
            return std::unique_ptr<Wolk>(new WolkExternal(m_device));
        }
        else
        {
            return std::unique_ptr<Wolk>(new WolkDefault(m_device));
        }
    }();

    // Setup protocols
    if (m_dataProtocol)
    {
        wolk->m_dataProtocol = std::move(m_dataProtocol);
    }
    else
    {
        wolk->m_dataProtocol.reset(new wolkabout::JsonProtocol(true));
    }
    wolk->m_gatewayDataProtocol.reset(new wolkabout::JsonGatewayDataProtocol());

    wolk->m_registrationProtocol.reset(new JsonRegistrationProtocol());
    wolk->m_gatewayRegistrationProtocol.reset(new JsonGatewaySubdeviceRegistrationProtocol());

    if (m_statusProtocol)
    {
        wolk->m_statusProtocol = std::move(m_statusProtocol);
    }
    else
    {
        wolk->m_statusProtocol.reset(new JsonStatusProtocol(true));
    }
    wolk->m_gatewayStatusProtocol.reset(new JsonGatewayStatusProtocol());

    wolk->m_firmwareUpdateProtocol.reset(new JsonDFUProtocol(true));
    wolk->m_gatewayFirmwareUpdateProtocol.reset(new JsonGatewayDFUProtocol());

    wolk->m_fileDownloadProtocol.reset(new JsonDownloadProtocol(true));

    // Setup device repository
    wolk->m_deviceRepository.reset(new SQLiteDeviceRepository());

    // Setup file repository
    wolk->m_fileRepository.reset(new SQLiteFileRepository(DATABASE));

    // Setup connectivity services
    wolk->m_platformConnectivityService.reset(new MqttConnectivityService(std::make_shared<PahoMqttClient>(),
                                                                          m_device.getKey(), m_device.getPassword(),
                                                                          m_platformHost, m_platformTrustStore));
    wolk->m_platformConnectivityService->setUncontrolledDisonnectMessage(
      wolk->m_statusProtocol->makeLastWillMessage(m_device.getKey()));

    wolk->m_platformPublisher.reset(new PublishingService(
      *wolk->m_platformConnectivityService, std::unique_ptr<GatewayPersistence>(new GatewayInMemoryPersistence())));

    wolk->m_inboundPlatformMessageHandler.reset(new GatewayInboundPlatformMessageHandler(m_device.getKey()));
    //    wolk->m_inboundDeviceMessageHandler.reset(new GatewayInboundDeviceMessageHandler());

    auto wolkRaw = wolk.get();

    wolk->m_platformConnectivityManager = std::make_shared<Wolk::ConnectivityFacade<InboundPlatformMessageHandler>>(
      *wolk->m_inboundPlatformMessageHandler, [=] { wolkRaw->platformDisconnected(); });

    //    wolk->m_deviceConnectivityManager = std::make_shared<Wolk::ConnectivityFacade<InboundDeviceMessageHandler>>(
    //      *wolk->m_inboundDeviceMessageHandler, [=] { wolk->devicesDisconnected(); });

    wolk->m_platformConnectivityService->setListener(wolk->m_platformConnectivityManager);
    //    wolk->m_deviceConnectivityService->setListener(wolk->m_deviceConnectivityManager);

    // Setup actuation and configuration handlers
    wolk->m_actuationHandlerLambda = m_actuationHandlerLambda;
    wolk->m_actuationHandler = m_actuationHandler;

    wolk->m_actuatorStatusProviderLambda = m_actuatorStatusProviderLambda;
    wolk->m_actuatorStatusProvider = m_actuatorStatusProvider;

    wolk->m_configurationHandlerLambda = m_configurationHandlerLambda;
    wolk->m_configurationHandler = m_configurationHandler;

    wolk->m_configurationProviderLambda = m_configurationProviderLambda;
    wolk->m_configurationProvider = m_configurationProvider;

    // Setup gateway update service
    wolk->m_gatewayUpdateService.reset(new GatewayUpdateService(m_device.getKey(), *wolk->m_registrationProtocol,
                                                                *wolk->m_deviceRepository, *wolk->m_platformPublisher));

    wolk->m_gatewayUpdateService->onGatewayUpdated([=] { wolkRaw->gatewayUpdated(); });

    if (m_externalDataProvider)
    {
        setupWithExternalData(dynamic_cast<WolkExternal*>(wolk.get()));
    }
    else
    {
        setupWithInternalData(dynamic_cast<WolkDefault*>(wolk.get()));
    }

    // setup file download service
    wolk->m_fileDownloadService =
      std::make_shared<FileDownloadService>(m_device.getKey(), *wolk->m_fileDownloadProtocol, m_fileDownloadDirectory,
                                            *wolk->m_platformPublisher, *wolk->m_fileRepository, m_urlFileDownloader);
    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_fileDownloadService);

    //    // setup firmware update service
    //    wolk->m_firmwareUpdateService = std::make_shared<FirmwareUpdateService>(
    //      m_device.getKey(), *wolk->m_firmwareUpdateProtocol, *wolk->m_gatewayFirmwareUpdateProtocol,
    //      *wolk->m_fileRepository, *wolk->m_platformPublisher, *wolk->m_devicePublisher, m_firmwareInstaller,
    //      m_firmwareVersion);
    //    wolk->m_inboundDeviceMessageHandler->addListener(wolk->m_firmwareUpdateService);
    //    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_firmwareUpdateService);

    return wolk;
}

void WolkBuilder::setupWithInternalData(WolkDefault* wolk)
{
    // Setup existing devices repository
    wolk->m_existingDevicesRepository.reset(new JsonFileExistingDevicesRepository());

    // connectivity
    const std::string localMqttClientId = std::string("Gateway-").append(m_device.getKey());
    wolk->m_deviceConnectivityService.reset(new MqttConnectivityService(
      std::make_shared<PahoMqttClient>(), m_device.getKey(), m_device.getPassword(), m_gatewayHost, localMqttClientId));

    wolk->m_devicePublisher.reset(new PublishingService(
      *wolk->m_deviceConnectivityService, std::unique_ptr<GatewayPersistence>(new GatewayInMemoryPersistence())));

    wolk->m_inboundDeviceMessageHandler.reset(new GatewayInboundDeviceMessageHandler());

    wolk->m_deviceConnectivityManager = std::make_shared<Wolk::ConnectivityFacade<InboundDeviceMessageHandler>>(
      *wolk->m_inboundDeviceMessageHandler, [=] { wolk->devicesDisconnected(); });

    wolk->m_deviceConnectivityService->setListener(wolk->m_deviceConnectivityManager);
    //

    // subdevice management
    if (m_device.getSubdeviceManagement().value() == SubdeviceManagement::GATEWAY)
    {
        // Setup registration service
        wolk->m_subdeviceRegistrationService.reset(new SubdeviceRegistrationService(
          m_device.getKey(), *wolk->m_registrationProtocol, *wolk->m_gatewayRegistrationProtocol,
          *wolk->m_deviceRepository, *wolk->m_platformPublisher, *wolk->m_devicePublisher));

        wolk->m_subdeviceRegistrationService->onDeviceRegistered(
          [=](const std::string& deviceKey) { wolk->deviceRegistered(deviceKey); });

        wolk->m_subdeviceRegistrationService->onDeviceUpdated(
          [=](const std::string& deviceKey) { wolk->deviceUpdated(deviceKey); });
    }

    wolk->m_registrationMessageRouter = std::make_shared<RegistrationMessageRouter>(
      *wolk->m_registrationProtocol, *wolk->m_gatewayRegistrationProtocol, wolk->m_gatewayUpdateService.get(),
      wolk->m_subdeviceRegistrationService.get(), wolk->m_subdeviceRegistrationService.get(),
      wolk->m_subdeviceRegistrationService.get(), wolk->m_subdeviceRegistrationService.get(),
      wolk->m_subdeviceRegistrationService.get());

    wolk->m_inboundDeviceMessageHandler->addListener(wolk->m_registrationMessageRouter);
    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_registrationMessageRouter);
    //

    // Setup device status service
    if (m_device.getSubdeviceManagement().value() == SubdeviceManagement::GATEWAY)
    {
        wolk->m_deviceStatusService.reset(new InternalDeviceStatusService(
          m_device.getKey(), *wolk->m_statusProtocol, *wolk->m_gatewayStatusProtocol, wolk->m_deviceRepository.get(),
          *wolk->m_platformPublisher, *wolk->m_devicePublisher, Wolk::KEEP_ALIVE_INTERVAL));
    }
    else
    {
        wolk->m_deviceStatusService.reset(new InternalDeviceStatusService(
          m_device.getKey(), *wolk->m_statusProtocol, *wolk->m_gatewayStatusProtocol, nullptr,
          *wolk->m_platformPublisher, *wolk->m_devicePublisher, Wolk::KEEP_ALIVE_INTERVAL));
    }

    // device data service
    if (m_device.getSubdeviceManagement().value() == SubdeviceManagement::GATEWAY)
    {
        wolk->m_dataService = std::make_shared<InternalDataService>(
          m_device.getKey(), *wolk->m_dataProtocol, *wolk->m_gatewayDataProtocol, wolk->m_deviceRepository.get(),
          *wolk->m_platformPublisher, *wolk->m_devicePublisher);
    }
    else
    {
        wolk->m_dataService =
          std::make_shared<InternalDataService>(m_device.getKey(), *wolk->m_dataProtocol, *wolk->m_gatewayDataProtocol,
                                                nullptr, *wolk->m_platformPublisher, *wolk->m_devicePublisher);
    }

    wolk->m_inboundDeviceMessageHandler->addListener(
      std::dynamic_pointer_cast<InternalDataService>(wolk->m_dataService));
    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_dataService);

    // gateway data service
    setupGatewayDataService(wolk, *wolk->m_dataService);

    wolk->m_dataService->setGatewayMessageListener(wolk->m_gatewayDataService.get());
    //

    // Setup keep alive service
    if (m_keepAliveEnabled)
    {
        wolk->m_keepAliveService.reset(new KeepAliveService(m_device.getKey(), *wolk->m_statusProtocol,
                                                            *wolk->m_platformPublisher, Wolk::KEEP_ALIVE_INTERVAL));
    }

    //     route device statuses if setup
    wolk->m_statusMessageRouter = std::make_shared<StatusMessageRouter>(
      *wolk->m_statusProtocol, *wolk->m_gatewayStatusProtocol, wolk->m_deviceStatusService.get(),
      wolk->m_deviceStatusService.get(), wolk->m_deviceStatusService.get(), wolk->m_keepAliveService.get());

    wolk->m_inboundDeviceMessageHandler->addListener(wolk->m_statusMessageRouter);
    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_statusMessageRouter);

    // setup firmware update service
    wolk->m_firmwareUpdateService = std::make_shared<FirmwareUpdateService>(
      m_device.getKey(), *wolk->m_firmwareUpdateProtocol, *wolk->m_gatewayFirmwareUpdateProtocol,
      *wolk->m_fileRepository, *wolk->m_platformPublisher, *wolk->m_devicePublisher, m_firmwareInstaller,
      m_firmwareVersion);
    wolk->m_inboundDeviceMessageHandler->addListener(wolk->m_firmwareUpdateService);
    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_firmwareUpdateService);
    //
}

void WolkBuilder::setupWithExternalData(WolkExternal* wolk)
{
    // Setup data service
    wolk->m_dataService = std::make_shared<ExternalDataService>(
      wolk->m_device.getKey(), *wolk->m_dataProtocol, *wolk->m_gatewayDataProtocol, *wolk->m_platformPublisher);

    // Setup device status service
    wolk->m_deviceStatusService.reset(
      new ExternalDeviceStatusService(m_device.getKey(), *wolk->m_statusProtocol, *wolk->m_platformPublisher));

    wolk->m_dataApi.reset(
      new DataHandlerApiFacade(dynamic_cast<ExternalDataService&>(*wolk->m_dataService), *wolk->m_deviceStatusService));

    m_externalDataProvider->setDataHandler(wolk->m_dataApi.get());

    setupGatewayDataService(wolk, *wolk->m_dataService);
    wolk->m_dataService->setGatewayMessageListener(wolk->m_gatewayDataService.get());
    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_dataService);

    // Setup keep alive service
    if (m_keepAliveEnabled)
    {
        wolk->m_keepAliveService.reset(new KeepAliveService(m_device.getKey(), *wolk->m_statusProtocol,
                                                            *wolk->m_platformPublisher, Wolk::KEEP_ALIVE_INTERVAL));
    }

    //     route device statuses if setup
    wolk->m_statusMessageRouter =
      std::make_shared<StatusMessageRouter>(*wolk->m_statusProtocol, *wolk->m_gatewayStatusProtocol, nullptr, nullptr,
                                            nullptr, wolk->m_keepAliveService.get());

    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_statusMessageRouter);

    //    // setup firmware update service
    //    wolk->m_firmwareUpdateService = std::make_shared<FirmwareUpdateService>(
    //      m_device.getKey(), *wolk->m_firmwareUpdateProtocol, *wolk->m_gatewayFirmwareUpdateProtocol,
    //      *wolk->m_fileRepository, *wolk->m_platformPublisher, *wolk->m_devicePublisher, m_firmwareInstaller,
    //      m_firmwareVersion);
    //    wolk->m_inboundDeviceMessageHandler->addListener(wolk->m_firmwareUpdateService);
    //    wolk->m_inboundPlatformMessageHandler->addListener(wolk->m_firmwareUpdateService);
}

void WolkBuilder::setupGatewayDataService(Wolk* wolk, OutboundMessageHandler& outboundMessageHandler)
{
    // setup gateway data service if gateway template is not empty
    const auto gwTemplate = m_device.getTemplate();
    if (!gwTemplate.getSensors().empty() || !gwTemplate.getActuators().empty() || !gwTemplate.getAlarms().empty() ||
        !gwTemplate.getConfigurations().empty())
    {
        auto wolkRaw = wolk;
        wolk->m_gatewayPersistence.reset(new InMemoryPersistence());
        wolk->m_gatewayDataService.reset(new GatewayDataService(
          m_device.getKey(), *wolk->m_dataProtocol, *wolk->m_gatewayPersistence, outboundMessageHandler,
          [wolkRaw](const std::string& reference, const std::string& value) {
              wolkRaw->handleActuatorSetCommand(reference, value);
          },
          [wolkRaw](const std::string& reference) { wolkRaw->handleActuatorGetCommand(reference); },
          [wolkRaw](const ConfigurationSetCommand& command) { wolkRaw->handleConfigurationSetCommand(command); },
          [wolkRaw] { wolkRaw->handleConfigurationGetCommand(); }));
    }
}

wolkabout::WolkBuilder::operator std::unique_ptr<Wolk>()
{
    return build();
}

WolkBuilder::WolkBuilder(GatewayDevice device)
: m_platformHost{WOLK_DEMO_HOST}, m_gatewayHost{MESSAGE_BUS_HOST}, m_device{std::move(device)}, m_keepAliveEnabled{true}
{
}

WolkBuilder::WolkBuilder(WolkBuilder&&) = default;
}    // namespace wolkabout
