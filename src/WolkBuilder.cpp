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
#include "DeviceManager.h"
#include "FileHandler.h"
#include "InboundDeviceMessageHandler.h"
#include "InboundPlatformMessageHandler.h"
#include "OutboundDataService.h"
#include "Wolk.h"
#include "connectivity/ConnectivityService.h"
#include "connectivity/ProtocolMapper.h"
#include "connectivity/json/RegistrationProtocol.h"
#include "connectivity/mqtt/MqttConnectivityService.h"
#include "connectivity/mqtt/PahoMqttClient.h"
#include "model/Device.h"
#include "persistence/inmemory/InMemoryPersistence.h"
#include "repository/SQLiteDeviceRepository.h"
#include "service/DeviceRegistrationService.h"
#include "service/PublishingService.h"

#include <stdexcept>

namespace wolkabout
{
WolkBuilder& WolkBuilder::host(const std::string& host)
{
    m_host = host;
    return *this;
}

WolkBuilder& WolkBuilder::withPersistence(std::shared_ptr<Persistence> persistence)
{
    m_persistence = persistence;
    return *this;
}

WolkBuilder& WolkBuilder::withFirmwareUpdate(const std::string& firmwareVersion,
                                             std::weak_ptr<FirmwareInstaller> installer,
                                             const std::string& firmwareDownloadDirectory,
                                             uint_fast64_t maxFirmwareFileSize,
                                             std::uint_fast64_t maxFirmwareFileChunkSize)
{
    return withFirmwareUpdate(firmwareVersion, installer, firmwareDownloadDirectory, maxFirmwareFileSize,
                              maxFirmwareFileChunkSize, std::weak_ptr<UrlFileDownloader>());
}

WolkBuilder& WolkBuilder::withFirmwareUpdate(const std::string& firmwareVersion,
                                             std::weak_ptr<FirmwareInstaller> installer,
                                             const std::string& firmwareDownloadDirectory,
                                             uint_fast64_t maxFirmwareFileSize,
                                             std::uint_fast64_t maxFirmwareFileChunkSize,
                                             std::weak_ptr<UrlFileDownloader> urlDownloader)
{
    m_firmwareVersion = firmwareVersion;
    m_firmwareDownloadDirectory = firmwareDownloadDirectory;
    m_maxFirmwareFileSize = maxFirmwareFileSize;
    m_maxFirmwareFileChunkSize = maxFirmwareFileChunkSize;
    m_firmwareInstaller = installer;
    m_urlFileDownloader = urlDownloader;
    return *this;
}

std::unique_ptr<Wolk> WolkBuilder::build() const
{
    if (m_device.getKey().empty())
    {
        throw std::logic_error("No device key present.");
    }

    auto wolk = std::unique_ptr<Wolk>(new Wolk(m_device));

    wolk->m_deviceManager.reset(new DeviceManager(std::unique_ptr<DeviceRepository>(new SQLiteDeviceRepository()),
                                                  MapProtocol(wolk->registerDataProtocol)));

    wolk->m_platformConnectivityService = std::make_shared<MqttConnectivityService>(
      std::make_shared<PahoMqttClient>(), m_device.getKey(), m_device.getPassword(), m_host);

    wolk->m_deviceConnectivityService = std::make_shared<MqttConnectivityService>(
      std::make_shared<PahoMqttClient>(), m_device.getKey(), m_device.getPassword(), "tcp://127.0.0.1:1883");

    wolk->m_platformPublisher = std::make_shared<PublishingService>(
      wolk->m_platformConnectivityService, std::unique_ptr<Persistence>(new InMemoryPersistence()));
    wolk->m_devicePublisher = std::make_shared<PublishingService>(
      wolk->m_deviceConnectivityService, std::unique_ptr<Persistence>(new InMemoryPersistence()));

    wolk->m_inboundPlatformMessageHandler = std::make_shared<InboundPlatformMessageHandler>(m_device.getKey());

    wolk->m_inboundDeviceMessageHandler = std::make_shared<InboundDeviceMessageHandler>();

    wolk->m_platformConnectivityManager =
      std::make_shared<Wolk::ConnectivityFacade>(*wolk->m_inboundPlatformMessageHandler, [&] {
          wolk->m_platformPublisher->disconnected();
          wolk->connectToPlatform();
      });

    wolk->m_deviceConnectivityManager =
      std::make_shared<Wolk::ConnectivityFacade>(*wolk->m_inboundDeviceMessageHandler, [&] {
          wolk->m_devicePublisher->disconnected();
          wolk->connectToDevices();
      });

    wolk->m_outboundServiceDataHandler =
      std::make_shared<OutboundDataService>(m_device, wolk->m_platformConnectivityService);

    wolk->m_platformConnectivityService->setListener(wolk->m_platformConnectivityManager);
    wolk->m_deviceConnectivityService->setListener(wolk->m_deviceConnectivityManager);

    wolk->m_deviceRegistrationService = std::make_shared<DeviceRegistrationService>(
      m_device.getKey(), *wolk->m_deviceManager, wolk->m_platformPublisher, wolk->m_devicePublisher);

    wolk->m_inboundDeviceMessageHandler->setListener<RegistrationProtocol>(wolk->m_deviceRegistrationService);
    wolk->m_inboundPlatformMessageHandler->setListener<RegistrationProtocol>(wolk->m_deviceRegistrationService);

    //	wolk->m_fileDownloadService = std::make_shared<FileDownloadService>(m_maxFirmwareFileSize,
    // m_maxFirmwareFileChunkSize,
    //																		std::unique_ptr<FileHandler>(new
    // FileHandler()),
    //																		outboundServiceDataHandler);

    //	if(m_firmwareInstaller.lock() != nullptr)
    //	{
    //		wolk->m_firmwareUpdateService = std::make_shared<FirmwareUpdateService>(m_firmwareVersion,
    // m_firmwareDownloadDirectory,
    //																				m_maxFirmwareFileSize,
    // outboundServiceDataHandler,
    //																				wolk->m_fileDownloadService,
    // m_urlFileDownloader,
    //																				m_firmwareInstaller);
    //	}

    // example
    if (MapProtocol(wolk->registerDataProtocol)("JsonSingle"))
    {
    }

    //	std::weak_ptr<FileDownloadService> fileDownloadService_weak{wolk->m_fileDownloadService};
    //	inboundMessageHandler->setBinaryDataHandler([=](const BinaryData& binaryData) -> void {
    //		if(auto handler = fileDownloadService_weak.lock())
    //		{
    //			handler->handleBinaryData(binaryData);
    //		}
    //	});

    //	std::weak_ptr<FirmwareUpdateService> firmwareUpdateService_weak{wolk->m_firmwareUpdateService};
    //	inboundMessageHandler->setFirmwareUpdateCommandHandler([=](const FirmwareUpdateCommand& firmwareUpdateCommand)
    //-> void {
    //		if(auto handler = firmwareUpdateService_weak.lock())
    //		{
    //			handler->handleFirmwareUpdateCommand(firmwareUpdateCommand);
    //		}
    //	});

    return wolk;
}

wolkabout::WolkBuilder::operator std::unique_ptr<Wolk>() const
{
    return build();
}

WolkBuilder::WolkBuilder(Device device)
: m_host{WOLK_DEMO_HOST}
, m_device{std::move(device)}
, m_persistence{new InMemoryPersistence()}
, m_firmwareVersion{""}
, m_firmwareDownloadDirectory{""}
, m_maxFirmwareFileSize{0}
, m_maxFirmwareFileChunkSize{0}
{
}
}    // namespace wolkabout
