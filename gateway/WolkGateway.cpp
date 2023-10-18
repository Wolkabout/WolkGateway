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

#include "core/connectivity/OutboundMessageHandler.h"
#include "core/connectivity/OutboundRetryMessageHandler.h"
#include "core/utility/Logger.h"
#include "gateway/connectivity/GatewayMessageRouter.h"
#include "gateway/repository/device/InMemoryDeviceRepository.h"
#include "gateway/service/devices/DevicesService.h"
#include "gateway/service/external_data/ExternalDataService.h"
#include "gateway/service/internal_data/InternalDataService.h"
#include "gateway/service/platform_status/GatewayPlatformStatusService.h"

#include <memory>
#include <thread>
#include <utility>

using namespace wolkabout::legacy;

namespace
{
const unsigned RECONNECT_DELAY_MSEC = 2000;
}

namespace wolkabout::gateway
{
WolkGateway::~WolkGateway() = default;

gateway::WolkGatewayBuilder WolkGateway::newBuilder(Device device)
{
    return gateway::WolkGatewayBuilder(std::move(device));
}

void WolkGateway::connect()
{
    connectLocal(true);
    connectPlatform(true);
}

void WolkGateway::disconnect()
{
    WolkSingle::disconnect();
    if (m_localConnectivityService != nullptr)
    {
        m_localConnectivityService->disconnect();
        m_localConnected = false;
    }
}

bool WolkGateway::isPlatformConnected()
{
    return WolkSingle::isConnected();
}

bool WolkGateway::isLocalConnected()
{
    if (m_localConnectivityService)
        return m_localConnected;
    return false;
}

void WolkGateway::publish()
{
    WolkInterface::publish();
}

connect::WolkInterfaceType WolkGateway::getType() const
{
    return connect::WolkInterfaceType::Gateway;
}

WolkGateway::WolkGateway(Device device)
: connect::WolkSingle(std::move(device))
, m_localConnected{false}
, m_localOutboundMessageHandler{nullptr}
, m_outboundMessageHandler{nullptr}
{
}

std::uint64_t WolkGateway::currentRtc()
{
    const auto duration = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

void WolkGateway::platformDisconnected()
{
    addToCommandBuffer([=] {
        notifyPlatformDisconnected();
        connectPlatform(true);
    });
}

void WolkGateway::notifyPlatformConnected()
{
    LOG(INFO) << "Connection to platform established";

    WolkSingle::notifyConnected();
    if (m_cacheDeviceRepository != nullptr)
        m_cacheDeviceRepository->loadInformationFromPersistentRepository();
    if (m_subdeviceManagementService != nullptr)
        m_subdeviceManagementService->updateDeviceCache();
    if (m_gatewayPlatformStatusService != nullptr)
        m_gatewayPlatformStatusService->sendPlatformConnectionStatusMessage(true);
    publish();
}

void WolkGateway::notifyPlatformDisconnected()
{
    LOG(INFO) << "Connection to platform lost";

    WolkSingle::notifyDisconnected();
    if (m_gatewayPlatformStatusService != nullptr)
        m_gatewayPlatformStatusService->sendPlatformConnectionStatusMessage(false);
}

void WolkGateway::connectPlatform(bool firstTime)
{
    addToCommandBuffer([=] {
        if (m_connectivityService == nullptr)
            return;

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
    addToCommandBuffer([=] {
        if (m_localConnectivityService == nullptr)
            return;

        if (firstTime)
            LOG(INFO) << TAG << "Connecting to local broker...";

        if (m_localConnectivityService->connect())
        {
            m_localConnected = true;
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
}    // namespace wolkabout::gateway
