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

#include "Wolk.h"
#include "InboundMessageHandler.h"
#include "WolkBuilder.h"
#include "connectivity/ConnectivityService.h"
#include "model/ConfigurationSetCommand.h"
#include "model/DetailedDevice.h"
#include "protocol/Protocol.h"
#include "repository/DeviceRepository.h"
#include "service/DataService.h"
#include "service/DeviceRegistrationService.h"
#include "service/FirmwareUpdateService.h"
#include "service/KeepAliveService.h"
#include "service/PublishingService.h"
#include "utilities/Logger.h"

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

WolkBuilder Wolk::newBuilder(Device device)
{
    return WolkBuilder(device);
}

void Wolk::connect()
{
    connectToPlatform();
    connectToDevices();
}

void Wolk::disconnect()
{
    addToCommandBuffer([=]() -> void { m_platformConnectivityService->disconnect(); });
    addToCommandBuffer([=]() -> void { m_deviceConnectivityService->disconnect(); });
}

void Wolk::addSensorReading(const std::string& reference, std::string value, unsigned long long rtc)
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

void Wolk::addSensorReading(const std::string& reference, const std::vector<std::string> values,
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
            m_gatewayDataService->addSensorReading(reference, values, getSensorDelimiter(reference), rtc);
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
            m_gatewayDataService->addConfiguration(configuration, getConfigurationDelimiters());
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

Wolk::Wolk(Device device) : m_device{device}
{
    m_commandBuffer = std::unique_ptr<CommandBuffer>(new CommandBuffer());
}

void Wolk::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(command));
}

unsigned long long Wolk::currentRtc()
{
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
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

void Wolk::publishFirmwareStatus()
{
    if (m_firmwareUpdateService)
    {
        m_firmwareUpdateService->reportFirmwareUpdateResult();
        m_firmwareUpdateService->publishFirmwareVersion();
    }
}

std::string Wolk::getSensorDelimiter(const std::string& reference)
{
    auto delimiters = m_device.getSensorDelimiters();

    auto it = delimiters.find(reference);
    if (it != delimiters.end())
    {
        return it->second;
    }

    return "";
}

std::map<std::string, std::string> Wolk::getConfigurationDelimiters()
{
    return m_device.getConfigurationDelimiters();
}

void Wolk::notifyPlatformConnected()
{
    m_platformPublisher->connected();

    if (m_keepAliveService)
    {
        m_keepAliveService->connected();
    }

    static bool shouldRegister = true;
    if (shouldRegister && m_deviceRegistrationService)
    {
        // register gateway upon first connect
        m_deviceRegistrationService->registerDevice(m_device);
        m_deviceRegistrationService->deleteDevicesOtherThan(m_existingDevicesRepository->getDeviceKeys());

        shouldRegister = false;
    }

    requestActuatorStatusesForDevices();
}

void Wolk::notifyPlatformDisonnected()
{
    m_platformPublisher->disconnected();

    if (m_keepAliveService)
    {
        m_keepAliveService->disconnected();
    }
}

void Wolk::notifyDevicesConnected()
{
    m_devicePublisher->connected();

    m_deviceStatusService->connected();
}

void Wolk::notifyDevicesDisonnected()
{
    m_devicePublisher->disconnected();

    m_deviceStatusService->disconnected();
}

void Wolk::connectToPlatform()
{
    addToCommandBuffer([=]() -> void {
        if (m_platformConnectivityService->connect())
        {
            notifyPlatformConnected();

            publishFirmwareStatus();

            for (const std::string& actuatorReference : m_device.getActuatorReferences())
            {
                publishActuatorStatus(actuatorReference);
            }

            publishConfiguration();

            publish();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY_MSEC));
            connectToPlatform();
        }
    });
}

void Wolk::connectToDevices()
{
    addToCommandBuffer([=]() -> void {
        if (m_deviceConnectivityService->connect())
        {
            notifyDevicesConnected();
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(RECONNECT_DELAY_MSEC));
            connectToDevices();
        }
    });
}

void Wolk::routePlatformData(const std::string& protocol, std::shared_ptr<Message> message)
{
    std::lock_guard<decltype(m_lock)> lg{m_lock};

    auto it = m_dataServices.find(protocol);
    if (it != m_dataServices.end())
    {
        std::get<0>(it->second)->platformMessageReceived(message);
    }
    else
    {
        LOG(WARN) << "Data service not found for protocol: " << protocol;
    }
}

void Wolk::routeDeviceData(const std::string& protocol, std::shared_ptr<Message> message)
{
    std::lock_guard<decltype(m_lock)> lg{m_lock};

    auto it = m_dataServices.find(protocol);
    if (it != m_dataServices.end())
    {
        std::get<0>(it->second)->deviceMessageReceived(message);
    }
    else
    {
        LOG(WARN) << "Data service not found for protocol: " << protocol;
    }
}

void Wolk::registerDataProtocol(std::shared_ptr<GatewayDataProtocol> protocol, std::shared_ptr<DataService> dataService)
{
    std::lock_guard<decltype(m_lock)> lg{m_lock};

    if (auto it = m_dataServices.find(protocol->getName()) != m_dataServices.end())
    {
        LOG(INFO) << "Data protocol already registered";
        return;
    }

    if (!dataService)
    {
        dataService = std::make_shared<DataService>(m_device.getKey(), *protocol, *m_deviceRepository,
                                                    *m_platformPublisher, *m_devicePublisher);
    }

    auto protocolResolver =
      std::make_shared<ChannelProtocolResolver>(*protocol, *m_deviceRepository,
                                                [&](const std::string& protocolName, std::shared_ptr<Message> message) {
                                                    routePlatformData(protocolName, message);
                                                },
                                                [&](const std::string& protocolName, std::shared_ptr<Message> message) {
                                                    routeDeviceData(protocolName, message);
                                                });

    m_dataServices[protocol->getName()] = std::make_tuple(dataService, protocol, protocolResolver);

    m_inboundDeviceMessageHandler->addListener(protocolResolver);
    m_inboundPlatformMessageHandler->addListener(protocolResolver);
}

void Wolk::requestActuatorStatusesForDevices()
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

void Wolk::requestActuatorStatusesForDevice(const std::string& deviceKey)
{
    std::lock_guard<decltype(m_lock)> lg{m_lock};

    auto device = m_deviceRepository->findByDeviceKey(deviceKey);
    if (!device)
    {
        LOG(ERROR) << "Device not found: " << deviceKey;
        return;
    }

    const auto protocol = device->getManifest().getProtocol();
    auto it = m_dataServices.find(protocol);
    if (it != m_dataServices.end())
    {
        std::get<0>(it->second)->requestActuatorStatusesForDevice(deviceKey);
    }
    else
    {
        LOG(WARN) << "Data service not found for protocol: " << protocol;
    }
}
}    // namespace wolkabout
