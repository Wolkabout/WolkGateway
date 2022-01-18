/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#include "WolkDefault.h"

#include "core/utilities/Logger.h"
#include "repository/existing_device/ExistingDevicesRepository.h"
#include "service/PublishingService.h"
#include "service/status/InternalDeviceStatusService.h"

namespace
{
const unsigned RECONNECT_DELAY_MSEC = 2000;
}

namespace wolkabout
{
namespace gateway
{
WolkDefault::~WolkDefault() = default;

void WolkDefault::connect()
{
    connectToDevices(true);
    connectToPlatform(true);
}

void WolkDefault::disconnect()
{
    addToCommandBuffer([=]() -> void { m_platformConnectivityService->disconnect(); });
    addToCommandBuffer([=]() -> void {
        if (m_deviceConnectivityService)
        {
            m_deviceConnectivityService->disconnect();
        }
    });
}

WolkDefault::WolkDefault(GatewayDevice device) : Wolk(device) {}

void WolkDefault::deviceRegistered(const std::string& deviceKey)
{
    addToCommandBuffer([=] {
        m_deviceStatusService->sendLastKnownStatusForDevice(deviceKey);
        m_existingDevicesRepository->addDeviceKey(deviceKey);
    });
}

void WolkDefault::deviceUpdated(const std::string& deviceKey)
{
    addToCommandBuffer([=] { m_deviceStatusService->sendLastKnownStatusForDevice(deviceKey); });
}

void WolkDefault::devicesDisconnected()
{
    addToCommandBuffer([=] {
        notifyDevicesDisonnected();
        connectToDevices(true);
    });
}

void WolkDefault::notifyDevicesConnected()
{
    LOG(INFO) << "Connection to local bus established";

    m_devicePublisher->connected();

    m_deviceStatusService->connected();
}

void WolkDefault::notifyDevicesDisonnected()
{
    LOG(INFO) << "Connection to local bus lost";

    m_devicePublisher->disconnected();

    m_deviceStatusService->disconnected();
}

void WolkDefault::connectToDevices(bool firstTime)
{
    if (!m_deviceConnectivityService)
    {
        return;
    }

    addToCommandBuffer([=]() -> void {
        if (firstTime)
            LOG(INFO) << "Connecting to local bus...";

        if (m_deviceConnectivityService->connect())
        {
            notifyDevicesConnected();
        }
        else
        {
            if (firstTime)
                LOG(INFO) << "Failed to connect to local bus";

            std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY_MSEC));
            connectToDevices(false);
        }
    });
}

}    // namespace gateway
}    // namespace wolkabout
