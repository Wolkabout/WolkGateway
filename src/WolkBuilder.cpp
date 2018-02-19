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
#include "ActuationHandler.h"
#include "ActuatorStatusProvider.h"
#include "Wolk.h"
#include "connectivity/ConnectivityService.h"
#include "connectivity/mqtt/MqttConnectivityService.h"
#include "connectivity/mqtt/PahoMqttClient.h"
#include "model/Device.h"
#include "InboundModuleMessageHandler.h"
#include "InboundWolkaboutMessageHandler.h"
#include "persistence/Persistence.h"
#include "persistence/inmemory/InMemoryPersistence.h"
#include "OutboundDataService.h"
#include "FileHandler.h"

#include "service/DataService.h"

#include <functional>
#include <stdexcept>
#include <string>

namespace wolkabout
{
WolkBuilder& WolkBuilder::host(const std::string& host)
{
    m_host = host;
    return *this;
}

WolkBuilder& WolkBuilder::actuationHandler(
  const std::function<void(const std::string&, const std::string&)>& actuationHandler)
{
    m_actuationHandlerLambda = actuationHandler;
    m_actuationHandler.reset();
    return *this;
}

WolkBuilder& WolkBuilder::actuationHandler(std::weak_ptr<ActuationHandler> actuationHandler)
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

WolkBuilder& WolkBuilder::actuatorStatusProvider(std::weak_ptr<ActuatorStatusProvider> actuatorStatusProvider)
{
    m_actuatorStatusProvider = actuatorStatusProvider;
    m_actuatorStatusProviderLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::withPersistence(std::shared_ptr<Persistence> persistence)
{
    m_persistence = persistence;
    return *this;
}

WolkBuilder& WolkBuilder::withFirmwareUpdate(const std::string& firmwareVersion, std::weak_ptr<FirmwareInstaller> installer,
											 const std::string& firmwareDownloadDirectory, uint_fast64_t maxFirmwareFileSize,
											 std::uint_fast64_t maxFirmwareFileChunkSize)
{
	return withFirmwareUpdate(firmwareVersion, installer, firmwareDownloadDirectory, maxFirmwareFileSize,
							  maxFirmwareFileChunkSize, std::weak_ptr<UrlFileDownloader>());
}

WolkBuilder& WolkBuilder::withFirmwareUpdate(const std::string& firmwareVersion, std::weak_ptr<FirmwareInstaller> installer,
											 const std::string& firmwareDownloadDirectory, uint_fast64_t maxFirmwareFileSize,
											 std::uint_fast64_t maxFirmwareFileChunkSize, std::weak_ptr<UrlFileDownloader> urlDownloader)
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
    if (m_device.getDeviceKey().empty())
    {
        throw std::logic_error("No device key present.");
    }

    if (m_device.getActuatorReferences().size() != 0)
    {
        if (m_actuationHandler.lock() == nullptr && m_actuationHandlerLambda == nullptr)
        {
            throw std::logic_error("Actuation handler not set.");
        }

        if (m_actuatorStatusProvider.lock() == nullptr && m_actuatorStatusProviderLambda == nullptr)
        {
            throw std::logic_error("Actuator status provider not set.");
        }
    }


	auto wolkConnectivityService = std::make_shared<MqttConnectivityService>(std::make_shared<PahoMqttClient>(), m_device, m_host);
	auto moduleConnectivityService = std::make_shared<MqttConnectivityService>(std::make_shared<PahoMqttClient>(), m_device, "tcp://127.0.0.1:1883");

	auto inboundModuleMessageHandler = std::make_shared<InboundModuleMessageHandler>();
	auto inboundWolkaboutMessageHandler = std::make_shared<InboundWolkaboutMessageHandler>(m_device.getDeviceKey());

	auto outboundServiceDataHandler = std::make_shared<OutboundDataService>(m_device, wolkConnectivityService);

	auto wolk = std::unique_ptr<Wolk>(new Wolk(wolkConnectivityService, moduleConnectivityService, m_persistence,
											   inboundWolkaboutMessageHandler, inboundModuleMessageHandler, outboundServiceDataHandler, m_device));

	wolk->m_wolkaboutConnectivityManager = std::make_shared<Wolk::ConnectivityFacade>(wolk->m_inboundWolkaboutMessageHandler.get(), [&]{
		wolk->connectToWolkabout();
	});

	wolk->m_moduleConnectivityManager = std::make_shared<Wolk::ConnectivityFacade>(wolk->m_inboundModuleMessageHandler.get(), [&]{
		wolk->connectToModules();
	});

	wolkConnectivityService->setListener(wolk->m_wolkaboutConnectivityManager);
	moduleConnectivityService->setListener(wolk->m_moduleConnectivityManager);

    wolk->m_actuationHandlerLambda = m_actuationHandlerLambda;
    wolk->m_actuationHandler = m_actuationHandler;

    wolk->m_actuatorStatusProviderLambda = m_actuatorStatusProviderLambda;
    wolk->m_actuatorStatusProvider = m_actuatorStatusProvider;

//	wolk->m_fileDownloadService = std::make_shared<FileDownloadService>(m_maxFirmwareFileSize, m_maxFirmwareFileChunkSize,
//																		std::unique_ptr<FileHandler>(new FileHandler()),
//																		outboundServiceDataHandler);

//	if(m_firmwareInstaller.lock() != nullptr)
//	{
//		wolk->m_firmwareUpdateService = std::make_shared<FirmwareUpdateService>(m_firmwareVersion, m_firmwareDownloadDirectory,
//																				m_maxFirmwareFileSize, outboundServiceDataHandler,
//																				wolk->m_fileDownloadService, m_urlFileDownloader,
//																				m_firmwareInstaller);
//	}


	std::weak_ptr<DataService> dataService_weak{wolk->m_dataService};

	inboundModuleMessageHandler->setSensorReadingHandler([=](Message reading){
		if(auto handler = dataService_weak.lock())
		{
			handler->handleSensorReading(reading);
		}
	});

	inboundModuleMessageHandler->setActuatorStatusHandler([=](Message status){
		if(auto handler = dataService_weak.lock())
		{
			handler->handleActuatorStatus(status);
		}
	});

//	std::weak_ptr<FileDownloadService> fileDownloadService_weak{wolk->m_fileDownloadService};
//	inboundMessageHandler->setBinaryDataHandler([=](const BinaryData& binaryData) -> void {
//		if(auto handler = fileDownloadService_weak.lock())
//		{
//			handler->handleBinaryData(binaryData);
//		}
//	});

//	std::weak_ptr<FirmwareUpdateService> firmwareUpdateService_weak{wolk->m_firmwareUpdateService};
//	inboundMessageHandler->setFirmwareUpdateCommandHandler([=](const FirmwareUpdateCommand& firmwareUpdateCommand) -> void {
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
	: m_host{WOLK_DEMO_HOST}, m_device{std::move(device)}, m_persistence{new InMemoryPersistence()},
	  m_firmwareVersion{""}, m_firmwareDownloadDirectory{""}, m_maxFirmwareFileSize{0}, m_maxFirmwareFileChunkSize{0}
{
}
}
