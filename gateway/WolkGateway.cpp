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

#include "gateway/WolkGateway.h"

#include "core/connectivity/ConnectivityService.h"
#include "core/connectivity/InboundPlatformMessageHandler.h"
#include "core/connectivity/OutboundMessageHandler.h"
#include "core/connectivity/OutboundRetryMessageHandler.h"
#include "core/persistence/Persistence.h"
#include "core/protocol/DataProtocol.h"
#include "core/protocol/ErrorProtocol.h"
#include "core/protocol/FileManagementProtocol.h"
#include "core/protocol/GatewayPlatformStatusProtocol.h"
#include "core/protocol/GatewayRegistrationProtocol.h"
#include "core/protocol/GatewaySubdeviceProtocol.h"
#include "core/protocol/RegistrationProtocol.h"
#include "core/utilities/CommandBuffer.h"
#include "core/utilities/Logger.h"
#include "gateway/connectivity/GatewayMessageRouter.h"
#include "gateway/repository/device/DeviceRepository.h"
#include "gateway/repository/existing_device/ExistingDevicesRepository.h"
#include "gateway/service/external_data/ExternalDataService.h"
#include "gateway/service/platform_status/GatewayPlatformStatusService.h"
#include "gateway/service/subdevice_management/SubdeviceManagementService.h"
#include "wolk/api/FeedUpdateHandler.h"
#include "wolk/api/ParameterHandler.h"
#include "wolk/service/data/DataService.h"
#include "wolk/service/error/ErrorService.h"
#include "wolk/service/file_management/FileManagementService.h"
#include "wolk/service/firmware_update/FirmwareUpdateService.h"
#include "wolk/service/platform_status/PlatformStatusService.h"
#include "wolk/service/registration_service/RegistrationService.h"

#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace
{
const unsigned RECONNECT_DELAY_MSEC = 2000;
}

namespace wolkabout
{
namespace gateway
{
WolkGateway::~WolkGateway() = default;

gateway::WolkGatewayBuilder WolkGateway::newBuilder(Device device)
{
    return gateway::WolkGatewayBuilder(std::move(device));
}

void WolkGateway::connect()
{
    connectPlatform(true);
    if (m_localConnectivityService != nullptr)
        connectLocal(true);
}

void WolkGateway::disconnect()
{
    WolkSingle::disconnect();
    if (m_localConnectivityService != nullptr)
        m_localConnectivityService->disconnect();
}

bool WolkGateway::isPlatformConnected()
{
    return WolkSingle::isConnected();
}

bool WolkGateway::isLocalConnected()
{
    if (m_localConnectivityService)
        return m_localConnectivityService->isConnected();
    return false;
}

void WolkGateway::publish()
{
    WolkInterface::publish();
}

void WolkGateway::requestDevices(const std::chrono::milliseconds& timestampFrom, const std::string& deviceType,
                                 const std::string& externalId)
{
    if (m_subdeviceManagementService == nullptr)
        return;

    addToCommandBuffer(
      [=]
      {
          if (m_subdeviceManagementService != nullptr)
              m_subdeviceManagementService->sendOutRegisteredDevicesRequest(timestampFrom, deviceType, externalId);
      });
}

connect::WolkInterfaceType WolkGateway::getType()
{
    return connect::WolkInterfaceType::Gateway;
}

WolkGateway::WolkGateway(Device device)
: connect::WolkSingle(std::move(device))
, m_connected{false}
, m_localOutboundMessageHandler{nullptr}
, m_outboundMessageHandler{nullptr}
{
    m_commandBuffer = std::unique_ptr<CommandBuffer>(new CommandBuffer());
}

std::uint64_t WolkGateway::currentRtc()
{
    const auto duration = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

void WolkGateway::handleFeedUpdate(const std::map<uint64_t, std::vector<Reading>>& readings)
{
    addToCommandBuffer(
      [=]
      {
          if (auto handler = m_feedUpdateHandler.lock())
              handler->handleUpdate(m_device.getKey(), readings);
          else if (m_feedUpdateHandlerLambda)
              m_feedUpdateHandlerLambda(m_device.getKey(), readings);
      });
}

void WolkGateway::handleParameterUpdate(const std::vector<Parameter>& parameters)
{
    addToCommandBuffer(
      [=]
      {
          if (auto handler = m_parameterHandler.lock())
              handler->handleUpdate(m_device.getKey(), parameters);
          else if (m_parameterLambda)
              m_parameterLambda(m_device.getKey(), parameters);
      });
}

void WolkGateway::platformDisconnected()
{
    addToCommandBuffer(
      [=]
      {
          notifyPlatformDisconnected();
          connectPlatform(true);
      });
}

void WolkGateway::notifyPlatformConnected()
{
    LOG(INFO) << "Connection to platform established";

    m_connected = true;
    if (m_platformConnectionStatusListener)
        m_platformConnectionStatusListener(m_connected);
    if (m_gatewayPlatformStatusService != nullptr)
        m_gatewayPlatformStatusService->sendPlatformConnectionStatusMessage(true);
}

void WolkGateway::notifyPlatformDisconnected()
{
    LOG(INFO) << "Connection to platform lost";

    m_connected = false;
    if (m_platformConnectionStatusListener)
        m_platformConnectionStatusListener(m_connected);
    if (m_gatewayPlatformStatusService != nullptr)
        m_gatewayPlatformStatusService->sendPlatformConnectionStatusMessage(false);
}

void WolkGateway::connectPlatform(bool firstTime)
{
    addToCommandBuffer(
      [=]
      {
          if (firstTime)
              LOG(INFO) << TAG << "Connecting to platform...";

          if (m_connectivityService->connect())
          {
              notifyPlatformConnected();
          }
          else
          {
              if (firstTime)
                  LOG(INFO) << TAG << "Failed to connect to platform.";

              std::this_thread::sleep_for(std::chrono::milliseconds{RECONNECT_DELAY_MSEC});
              connectPlatform();
          }
      });
}

void WolkGateway::connectLocal(bool firstTime)
{
    addToCommandBuffer(
      [=]
      {
          if (firstTime)
              LOG(INFO) << TAG << "Connecting to local broker...";

          if (m_connectivityService->connect())
          {
          }
          else
          {
              if (firstTime)
                  LOG(INFO) << TAG << "Failed to connect to local broker.";

              std::this_thread::sleep_for(std::chrono::milliseconds{RECONNECT_DELAY_MSEC});
              connectLocal();
          }
      });
}
}    // namespace gateway
}    // namespace wolkabout
