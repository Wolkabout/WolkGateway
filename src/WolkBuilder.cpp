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
#include "protocol/json/JsonGatewayDataProtocol.h"
#include "protocol/json/JsonGatewayDeviceRegistrationProtocol.h"
#include "protocol/json/JsonGatewayStatusProtocol.h"
#include "protocol/json/JsonProtocol.h"
#include "repository/ExistingDevicesRepository.h"
#include "repository/JsonFileExistingDevicesRepository.h"
#include "repository/SQLiteDeviceRepository.h"
#include "service/DeviceRegistrationService.h"
#include "service/DeviceStatusService.h"
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

WolkBuilder& WolkBuilder::withDataProtocol(std::shared_ptr<GatewayDataProtocol> protocol)
{
    m_dataProtocol = protocol;
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

    auto wolk = std::unique_ptr<Wolk>(new Wolk(m_device));

    wolk->m_statusProtocol = std::unique_ptr<GatewayStatusProtocol>(new JsonGatewayStatusProtocol());
    wolk->m_registrationProtocol =
      std::unique_ptr<GatewayDeviceRegistrationProtocol>(new JsonGatewayDeviceRegistrationProtocol());

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

    // register gateway upon start
    wolk->m_deviceRegistrationService->registerDevice(m_device);

    wolk->m_deviceRegistrationService->deleteDevicesOtherThan(wolk->m_existingDevicesRepository->getDeviceKeys());

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
