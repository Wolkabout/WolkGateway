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

void InMemoryDeviceRepository::loadLatestTimestampFromPersistentRepository()
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};

    if (m_persistentDeviceRepository != nullptr)
    {
        const auto loadedTimestamp = m_persistentDeviceRepository->latestTimestamp();
        if (loadedTimestamp > m_timestamp)
            m_timestamp = loadedTimestamp;
    }
}

bool InMemoryDeviceRepository::save(std::chrono::milliseconds timestamp, const RegisteredDeviceInformation& device)
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};

    // Save the value to the local vector
    if (!containsDeviceKey(device.deviceKey))
        m_devices.emplace_back(device.deviceKey);

    // Update the timestamp
    if (timestamp > m_timestamp)
        m_timestamp = timestamp;

    // If the persistent repository is present, tell it to save data too
    if (m_persistentDeviceRepository != nullptr && m_commandBuffer != nullptr)
        m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(
          [this, timestamp, device] { m_persistentDeviceRepository->save(timestamp, device); }));
    return true;
}

bool InMemoryDeviceRepository::remove(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};

    // Empty the local vector
    const auto deviceKeyIt = std::find(m_devices.cbegin(), m_devices.cend(), deviceKey);
    if (deviceKeyIt != m_devices.cend())
        m_devices.erase(deviceKeyIt);

    // If we have access to more permanent persistence, delete it too
    if (m_persistentDeviceRepository != nullptr && m_commandBuffer != nullptr)
    {
        auto deviceKeyCopy = std::string{deviceKey};
        m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(
          [this, deviceKeyCopy] { m_persistentDeviceRepository->remove(deviceKeyCopy); }));
    }
    return true;
}

bool InMemoryDeviceRepository::removeAll()
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};

    // Empty the local vector
    m_devices.clear();

    // If we have access to more permanent persistence, delete it too
    if (m_persistentDeviceRepository != nullptr && m_commandBuffer != nullptr)
        m_commandBuffer->pushCommand(
          std::make_shared<std::function<void()>>([this] { m_persistentDeviceRepository->removeAll(); }));
    return true;
}

bool InMemoryDeviceRepository::containsDeviceKey(const std::string& deviceKey)
{
    LOG(TRACE) << METHOD_INFO;
    std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};

    // Check if it is in the vector
    const auto deviceKeyIt = std::find(m_devices.cbegin(), m_devices.cend(), deviceKey);
    if (deviceKeyIt != m_devices.cend())
        return true;

    // Check if we can try to see if it is in permanent persistence
    if (m_persistentDeviceRepository == nullptr)
        return false;
    const auto permanentPersistenceContains = m_persistentDeviceRepository->containsDeviceKey(deviceKey);
    // Save it in our local persistence if it is contained in there
    if (permanentPersistenceContains)
        m_devices.emplace_back(deviceKey);
    return permanentPersistenceContains;
}

std::chrono::milliseconds InMemoryDeviceRepository::latestTimestamp()
{
    std::lock_guard<std::recursive_mutex> lockGuard{m_mutex};
    return m_timestamp;
}
}    // namespace gateway
}    // namespace wolkabout
