/**
 * Copyright 2022 WolkAbout Technology s.r.o.
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

#include "gateway/repository/device/InMemoryDeviceRepository.h"

#include "core/utilities/Logger.h"

#include <algorithm>

namespace wolkabout
{
namespace gateway
{
InMemoryDeviceRepository::InMemoryDeviceRepository(std::shared_ptr<DeviceRepository> persistentDeviceRepository)
: m_timestamp{0}
, m_persistentDeviceRepository{std::move(persistentDeviceRepository)}
, m_commandBuffer{m_persistentDeviceRepository != nullptr ? new CommandBuffer : nullptr}
{
}

void InMemoryDeviceRepository::loadInformationFromPersistentRepository()
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};

    if (m_persistentDeviceRepository != nullptr)
    {
        // Copy all the gateway devices
        const auto gatewayDevices = m_persistentDeviceRepository->getGatewayDevices();
        std::copy(gatewayDevices.cbegin(), gatewayDevices.cend(), std::back_inserter(m_devices));

        // Copy the timestamp
        const auto loadedTimestamp = m_persistentDeviceRepository->latestPlatformTimestamp();
        if (loadedTimestamp > m_timestamp)
            m_timestamp = loadedTimestamp;
    }
}

bool InMemoryDeviceRepository::save(const std::vector<StoredDeviceInformation>& devices)
{
    // Store the devices info
    {
        std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};
        for (const auto& device : devices)
        {
            // Save the value to the local vector
            if (!containsDevice(device.getDeviceKey()))
                m_devices.emplace_back(device);

            // Update the timestamp
            if (device.getTimestamp() > m_timestamp)
                m_timestamp = device.getTimestamp();
        }
    }

    // If the persistent repository is present, tell it to save data too
    if (m_persistentDeviceRepository != nullptr && m_commandBuffer != nullptr)
        m_commandBuffer->pushCommand(
          std::make_shared<std::function<void()>>([this, devices] { m_persistentDeviceRepository->save(devices); }));
    return true;
}

bool InMemoryDeviceRepository::remove(const std::vector<std::string>& deviceKeys)
{
    // Empty the local vector
    {
        std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};
        for (const auto& deviceKey : deviceKeys)
        {
            const auto it = std::find_if(
              m_devices.begin(), m_devices.end(),
              [&](const StoredDeviceInformation& information) { return information.getDeviceKey() == deviceKey; });
            if (it != m_devices.cend())
                m_devices.erase(it);
        }
    }

    // If we have access to more permanent persistence, delete it too
    if (m_persistentDeviceRepository != nullptr && m_commandBuffer != nullptr)
    {
        m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(
          [this, deviceKeys] { m_persistentDeviceRepository->remove(deviceKeys); }));
    }
    return true;
}

bool InMemoryDeviceRepository::removeAll()
{
    // Empty the local vector
    {
        std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};
        m_devices.clear();
    }

    // If we have access to more permanent persistence, delete it too
    if (m_persistentDeviceRepository != nullptr && m_commandBuffer != nullptr)
        m_commandBuffer->pushCommand(
          std::make_shared<std::function<void()>>([this] { m_persistentDeviceRepository->removeAll(); }));
    return true;
}

bool InMemoryDeviceRepository::containsDevice(const std::string& deviceKey)
{
    bool found;

    // Check in local memory
    {
        std::lock_guard<std::recursive_mutex> lock{m_mutex};
        const auto it = std::find_if(
          m_devices.begin(), m_devices.end(),
          [&](const StoredDeviceInformation& information) { return information.getDeviceKey() == deviceKey; });
        found = it != m_devices.cend();
    }

    // If not found, check in persistent storage
    if (!found && m_persistentDeviceRepository != nullptr)
    {
        auto returningInformation = m_persistentDeviceRepository->get(deviceKey);
        if (!returningInformation.getDeviceKey().empty())
        {
            m_devices.emplace_back(returningInformation);
            found = true;
        }
    }
    return found;
}

StoredDeviceInformation InMemoryDeviceRepository::get(const std::string& deviceKey)
{
    auto returningInformation = StoredDeviceInformation{};

    // Check in local memory
    {
        std::lock_guard<std::recursive_mutex> lock{m_mutex};
        const auto it = std::find_if(
          m_devices.begin(), m_devices.end(),
          [&](const StoredDeviceInformation& information) { return information.getDeviceKey() == deviceKey; });
        if (it != m_devices.cend())
            returningInformation = StoredDeviceInformation{*it};
    }

    // If not found, check in persistent storage
    if (returningInformation.getDeviceKey().empty() && m_persistentDeviceRepository != nullptr)
    {
        returningInformation = m_persistentDeviceRepository->get(deviceKey);
        if (!returningInformation.getDeviceKey().empty())
            m_devices.emplace_back(returningInformation);
    }
    return returningInformation;
}

std::vector<StoredDeviceInformation> InMemoryDeviceRepository::getGatewayDevices()
{
    auto gatewayDevices = std::vector<StoredDeviceInformation>{};
    {
        std::lock_guard<std::recursive_mutex> lock{m_mutex};
        std::copy_if(m_devices.cbegin(), m_devices.cend(), std::back_inserter(gatewayDevices),
                     [&](const StoredDeviceInformation& information) {
                         return information.getDeviceBelongsTo() == DeviceOwnership::Gateway;
                     });
    }
    return gatewayDevices;
}

std::chrono::milliseconds InMemoryDeviceRepository::latestPlatformTimestamp()
{
    std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};
    return m_timestamp;
}
}    // namespace gateway
}    // namespace wolkabout
