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

#include "gateway/WolkBuilder.h"

#include "core/connectivity/InboundPlatformMessageHandler.h"
#include "core/connectivity/mqtt/MqttConnectivityService.h"
#include "core/connectivity/mqtt/PahoMqttClient.h"
#include "core/protocol/wolkabout/WolkaboutDataProtocol.h"
#include "core/protocol/wolkabout/WolkaboutErrorProtocol.h"
#include "core/protocol/wolkabout/WolkaboutFileManagementProtocol.h"
#include "core/protocol/wolkabout/WolkaboutFirmwareUpdateProtocol.h"
#include "core/protocol/wolkabout/WolkaboutGatewaySubdeviceProtocol.h"
#include "core/protocol/wolkabout/WolkaboutPlatformStatusProtocol.h"
#include "core/protocol/wolkabout/WolkaboutRegistrationProtocol.h"
#include "gateway/WolkGateway.h"
#include "gateway/connectivity/GatewayMessageRouter.h"
#include "gateway/persistence/inmemory/GatewayInMemoryPersistence.h"

#include <memory>
#include <stdexcept>

namespace wolkabout
{
namespace gateway
{
WolkBuilder::WolkBuilder(Device device)
: m_device{std::move(device)}
, m_platformHost{WOLK_DEMO_HOST}
, m_platformMqttKeepAliveSec{60}
, m_gatewayHost{MESSAGE_BUS_HOST}
, m_gatewayPersistence{new GatewayInMemoryPersistence}
, m_dataProtocol{new WolkaboutDataProtocol}
, m_errorProtocol{new WolkaboutErrorProtocol}
, m_errorRetainTime{std::chrono::seconds{1}}
, m_gatewaySubdeviceProtocol{new WolkaboutGatewaySubdeviceProtocol}
, m_fileTransferEnabled{false}
, m_fileTransferUrlEnabled{false}
, m_maxPacketSize{MAX_PACKET_SIZE}
{
}

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

WolkBuilder& WolkBuilder::feedUpdateHandler(
  const std::function<void(std::string, const std::map<std::uint64_t, std::vector<Reading>>)>& feedUpdateHandler)
{
    m_feedUpdateHandlerLambda = feedUpdateHandler;
    m_feedUpdateHandler.reset();
    return *this;
}

WolkBuilder& WolkBuilder::feedUpdateHandler(std::weak_ptr<connect::FeedUpdateHandler> feedUpdateHandler)
{
    m_feedUpdateHandler = std::move(feedUpdateHandler);
    m_feedUpdateHandlerLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::parameterHandler(
  const std::function<void(std::string, std::vector<Parameter>)>& parameterHandlerLambda)
{
    m_parameterHandlerLambda = parameterHandlerLambda;
    m_parameterHandler.reset();
    return *this;
}

WolkBuilder& WolkBuilder::parameterHandler(std::weak_ptr<connect::ParameterHandler> parameterHandler)
{
    m_parameterHandler = std::move(parameterHandler);
    m_parameterHandlerLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::withPersistence(std::unique_ptr<GatewayPersistence> persistence)
{
    m_gatewayPersistence = std::move(persistence);
    return *this;
}

WolkBuilder& WolkBuilder::withDataProtocol(std::unique_ptr<DataProtocol> protocol)
{
    m_dataProtocol = std::move(protocol);
    return *this;
}

WolkBuilder& WolkBuilder::withErrorProtocol(std::chrono::milliseconds errorRetainTime,
                                            std::unique_ptr<ErrorProtocol> protocol)
{
    m_errorRetainTime = errorRetainTime;
    if (protocol != nullptr)
        m_errorProtocol = std::move(protocol);
    return *this;
}

WolkBuilder& WolkBuilder::withFileTransfer(const std::string& fileDownloadLocation, std::uint64_t maxPacketSize)
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

WolkBuilder& WolkBuilder::withFileURLDownload(const std::string& fileDownloadLocation,
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

WolkBuilder& WolkBuilder::withFileListener(const std::shared_ptr<connect::FileListener>& fileListener)
{
    m_fileListener = fileListener;
    return *this;
}

WolkBuilder& WolkBuilder::withFirmwareUpdate(std::unique_ptr<connect::FirmwareInstaller> firmwareInstaller,
                                             const std::string& workingDirectory)
{
    if (m_firmwareUpdateProtocol == nullptr)
        m_firmwareUpdateProtocol =
          std::unique_ptr<WolkaboutFirmwareUpdateProtocol>(new wolkabout::WolkaboutFirmwareUpdateProtocol);
    m_firmwareParametersListener = nullptr;
    m_firmwareInstaller = std::move(firmwareInstaller);
    m_workingDirectory = workingDirectory;
    return *this;
}

WolkBuilder& WolkBuilder::withFirmwareUpdate(
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

WolkBuilder& WolkBuilder::setMqttKeepAlive(std::uint16_t keepAlive)
{
    m_platformMqttKeepAliveSec = keepAlive;
    return *this;
}

std::unique_ptr<WolkGateway> WolkBuilder::build()
{
    // Right away check a bunch of the parameters
    if (m_device.getKey().empty())
        throw std::logic_error("No device key present.");

    // Create the instance of the Wolk based on the data provider source.
    auto wolk = std::unique_ptr<WolkGateway>{new WolkGateway{m_device}};
    auto wolkRaw = wolk.get();

    // Create the platform connection
    auto mqttClient = std::make_shared<PahoMqttClient>();
    wolk->m_connectivityService = std::unique_ptr<MqttConnectivityService>{new MqttConnectivityService{
      std::move(mqttClient), m_device.getKey(), m_device.getPassword(), m_platformHost, m_platformTrustStore,
      ByteUtils::toUUIDString(ByteUtils::generateRandomBytes(ByteUtils::UUID_VECTOR_SIZE))}};

    // Set up the connection links
    wolk->m_inboundMessageHandler =
      std::make_shared<InboundPlatformMessageHandler>(std::vector<std::string>{m_device.getKey()});
    wolk->m_connectivityService->onConnectionLost([wolkRaw] {
        wolkRaw->notifyPlatformDisconnected();
        wolkRaw->connectPlatform(true);
    });
    wolk->m_connectivityService->setListner(wolk->m_inboundMessageHandler);

    // Set up the gateway message router
    wolk->m_gatewaySubdeviceProtocol = std::move(m_gatewaySubdeviceProtocol);
    wolk->m_gatewayMessageRouter = std::make_shared<GatewayMessageRouter>(*wolk->m_gatewaySubdeviceProtocol);
    wolk->m_inboundMessageHandler->addListener(wolk->m_gatewayMessageRouter);

    // Set up the protocols
    wolk->m_dataProtocol = std::move(m_dataProtocol);
    wolk->m_errorProtocol = std::move(m_errorProtocol);

    return wolk;
}

WolkBuilder::operator std::unique_ptr<WolkGateway>()
{
    return build();
}
}    // namespace gateway
}    // namespace wolkabout
