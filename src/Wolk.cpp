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

#include "Wolk.h"

#include "core/connectivity/ConnectivityService.h"
#include "core/model/ConfigurationSetCommand.h"
#include "core/persistence/Persistence.h"
#include "core/protocol/DataProtocol.h"
#include "core/protocol/RegistrationProtocol.h"
#include "core/protocol/StatusProtocol.h"
#include "core/protocol/json/JsonDFUProtocol.h"
#include "core/protocol/json/JsonDownloadProtocol.h"
#include "core/utilities/Logger.h"
#include "protocol/GatewayDataProtocol.h"
#include "protocol/GatewayFirmwareUpdateProtocol.h"
#include "protocol/GatewayStatusProtocol.h"
#include "protocol/GatewaySubdeviceRegistrationProtocol.h"
#include "repository/DeviceRepository.h"
#include "repository/ExistingDevicesRepository.h"
#include "repository/FileRepository.h"
#include "service/FileDownloadService.h"
#include "service/FirmwareUpdateService.h"
#include "service/GatewayUpdateService.h"
#include "service/KeepAliveService.h"
#include "service/PublishingService.h"
#include "service/SubdeviceRegistrationService.h"
#include "service/data/DataService.h"
#include "service/data/GatewayDataService.h"
#include "service/status/DeviceStatusService.h"

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
const constexpr std::chrono::seconds Wolk::KEEP_ALIVE_INTERVAL;

WolkBuilder Wolk::newBuilder(GatewayDevice device)
{
    return WolkBuilder(std::move(device));
}

bool Wolk::isConnectedToPlatform()
{
    return m_connected;
}

void Wolk::setPlatformConnectionStatusListener(const std::function<void(bool)>& platformConnectionStatusListener)
{
    this->m_platformConnectionStatusListener = platformConnectionStatusListener;
}

void Wolk::addSensorReading(const std::string& reference, const std::string& value, unsigned long long rtc)
{
    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    addToCommandBuffer([=]() -> void {
        if (m_gatewayDataService)
        {
            m_gatewayDataService->addSensorReading(reference, value, rtc);
        }
    });
}

void Wolk::addSensorReading(const std::string& reference, const std::vector<std::string>& values,
                            unsigned long long int rtc)
{
    if (values.empty())
    {
        return;
    }

    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    addToCommandBuffer([=]() -> void {
        if (m_gatewayDataService)
        {
            m_gatewayDataService->addSensorReading(reference, values, rtc);
        }
    });
}

void Wolk::addAlarm(const std::string& reference, bool active, unsigned long long rtc)
{
    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    addToCommandBuffer([=]() -> void {
        if (m_gatewayDataService)
        {
            m_gatewayDataService->addAlarm(reference, active, rtc);
        }
    });
}

void Wolk::publishActuatorStatus(const std::string& reference)
{
    addToCommandBuffer([=]() -> void {
        const ActuatorStatus actuatorStatus = [&]() -> ActuatorStatus {
            if (auto provider = m_actuatorStatusProvider.lock())
            {
                return provider->getActuatorStatus(reference);
            }
            else if (m_actuatorStatusProviderLambda)
            {
                return m_actuatorStatusProviderLambda(reference);
            }

            return ActuatorStatus();
        }();

        if (m_gatewayDataService)
        {
            m_gatewayDataService->addActuatorStatus(reference, actuatorStatus.getValue(), actuatorStatus.getState());
        }
        flushActuatorStatuses();
    });
}

void Wolk::publishConfiguration()
{
    addToCommandBuffer([=]() -> void {
        const auto configuration = [=]() -> std::vector<ConfigurationItem> {
            if (auto provider = m_configurationProvider.lock())
            {
                return provider->getConfiguration();
            }
            else if (m_configurationProviderLambda)
            {
                return m_configurationProviderLambda();
            }

            return std::vector<ConfigurationItem>();
        }();

        if (m_gatewayDataService)
        {
            m_gatewayDataService->addConfiguration(configuration);
        }
        flushConfiguration();
    });
}

void Wolk::publish()
{
    addToCommandBuffer([=]() -> void {
        flushActuatorStatuses();
        flushAlarms();
        flushSensorReadings();
        flushConfiguration();
    });
}

Wolk::Wolk(GatewayDevice device) : m_device{device}
{
    m_commandBuffer = std::unique_ptr<CommandBuffer>(new CommandBuffer());
}

Wolk::~Wolk() = default;

void Wolk::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(command));
}

unsigned long long Wolk::currentRtc()
{
    auto duration = std::chrono::high_resolution_clock::now().time_since_epoch();
    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
}

void Wolk::flushActuatorStatuses()
{
    if (m_gatewayDataService)
    {
        m_gatewayDataService->publishActuatorStatuses();
    }
}

void Wolk::flushAlarms()
{
    if (m_gatewayDataService)
    {
        m_gatewayDataService->publishAlarms();
    }
}

void Wolk::flushSensorReadings()
{
    if (m_gatewayDataService)
    {
        m_gatewayDataService->publishSensorReadings();
    }
}

void Wolk::flushConfiguration()
{
    if (m_gatewayDataService)
    {
        m_gatewayDataService->publishConfiguration();
    }
}

void Wolk::handleActuatorSetCommand(const std::string& reference, const std::string& value)
{
    addToCommandBuffer([=] {
        if (auto provider = m_actuationHandler.lock())
        {
            provider->handleActuation(reference, value);
        }
        else if (m_actuationHandlerLambda)
        {
            m_actuationHandlerLambda(reference, value);
        }
    });

    publishActuatorStatus(reference);
}

void Wolk::handleActuatorGetCommand(const std::string& reference)
{
    publishActuatorStatus(reference);
}

void Wolk::handleConfigurationSetCommand(const ConfigurationSetCommand& command)
{
    addToCommandBuffer([=]() -> void {
        if (auto handler = m_configurationHandler.lock())
        {
            handler->handleConfiguration(command.getValues());
        }
        else if (m_configurationHandlerLambda)
        {
            m_configurationHandlerLambda(command.getValues());
        }
    });

    publishConfiguration();
}

void Wolk::handleConfigurationGetCommand()
{
    publishConfiguration();
}

void Wolk::platformDisconnected()
{
    addToCommandBuffer([=] {
        notifyPlatformDisonnected();
        connectToPlatform(true);
    });
}

void Wolk::gatewayUpdated()
{
    addToCommandBuffer([=] {
        if (m_keepAliveService)
        {
            m_keepAliveService->sendPingMessage();
        }

        publishEverything();

        if (m_subdeviceRegistrationService && m_device.getSubdeviceManagement().value() == SubdeviceManagement::GATEWAY)
        {
            m_subdeviceRegistrationService->registerPostponedDevices();
            m_subdeviceRegistrationService->updatePostponedDevices();
        }
    });
}

void Wolk::publishEverything()
{
    publishFirmwareStatus();

    publishConfiguration();

    for (const std::string& actuatorReference : m_device.getActuatorReferences())
    {
        publishActuatorStatus(actuatorReference);
    }

    publishFileList();
}

void Wolk::publishFirmwareStatus()
{
    if (m_firmwareUpdateService)
    {
        m_firmwareUpdateService->reportFirmwareUpdateResult();
        m_firmwareUpdateService->publishFirmwareVersion();
    }
}

void Wolk::publishFileList()
{
    if (m_fileDownloadService)
    {
        m_fileDownloadService->sendFileList();
    }
}

void Wolk::updateGatewayAndDeleteDevices()
{
    static bool shouldUpdate = true;

    // update gateway upon first connect
    if (shouldUpdate && m_subdeviceRegistrationService &&
        m_device.getSubdeviceManagement().value() == SubdeviceManagement::GATEWAY)
    {
        // This call is now intentionally disabled because we don't want to update the gateway ever.
        //        m_gatewayUpdateService->updateGateway(m_device);
        shouldUpdate = false;

        if (m_subdeviceRegistrationService && m_device.getSubdeviceManagement().value() == SubdeviceManagement::GATEWAY)
        {
            m_subdeviceRegistrationService->deleteDevicesOtherThan(m_existingDevicesRepository->getDeviceKeys());
        }
    }
}

void Wolk::notifyPlatformConnected()
{
    LOG(INFO) << "Connection to platform established";

    m_connected = true;
    if (m_platformConnectionStatusListener)
        m_platformConnectionStatusListener(m_connected);

    m_platformPublisher->connected();

    if (m_keepAliveService)
    {
        m_keepAliveService->connected();
    }
}

void Wolk::notifyPlatformDisonnected()
{
    LOG(INFO) << "Connection to platform lost";

    m_connected = false;
    if (m_platformConnectionStatusListener)
        m_platformConnectionStatusListener(m_connected);

    m_platformPublisher->disconnected();

    if (m_keepAliveService)
    {
        m_keepAliveService->disconnected();
    }
}

void Wolk::connectToPlatform(bool firstTime)
{
    addToCommandBuffer([=]() -> void {
        if (firstTime)
            LOG(INFO) << "Connecting to platform...";

        if (m_platformConnectivityService->connect())
        {
            notifyPlatformConnected();

            updateGatewayAndDeleteDevices();

            requestActuatorStatusesForDevices();

            publishEverything();

            publish();
        }
        else
        {
            if (firstTime)
                LOG(INFO) << "Failed to connect to platform";

            std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY_MSEC));
            connectToPlatform(false);
        }
    });
}

void Wolk::requestActuatorStatusesForDevices()
{
    if (m_device.getSubdeviceManagement().value() == SubdeviceManagement::GATEWAY)
    {
        auto keys = m_deviceRepository->findAllDeviceKeys();
        if (keys)
        {
            for (const auto& key : *keys)
            {
                if (key == m_device.getKey())
                {
                    continue;
                }
                requestActuatorStatusesForDevice(key);
            }
        }
    }
    else
    {
        std::lock_guard<decltype(m_lock)> lg{m_lock};
        m_dataService->requestActuatorStatusesForAllDevices();
    }
}

void Wolk::requestActuatorStatusesForDevice(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_lock)> lg{m_lock};
    m_dataService->requestActuatorStatusesForDevice(deviceKey);
}
}    // namespace wolkabout
