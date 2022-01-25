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
#ifndef WOLKGATEWAY_INMEMORYDEVICEREPOSITORY_H
#define WOLKGATEWAY_INMEMORYDEVICEREPOSITORY_H

#include "core/utilities/CommandBuffer.h"
#include "gateway/repository/device/DeviceRepository.h"

#include <unordered_map>

namespace wolkabout
{
namespace gateway
{
class InMemoryDeviceRepository : public DeviceRepository
{
public:
    /**
     * Default parameter constructor for this repository.
     *
     * @param persistentDeviceRepository An optional persistent device repository.
     */
    explicit InMemoryDeviceRepository(std::shared_ptr<DeviceRepository> persistentDeviceRepository = nullptr);

    /**
     * This method will load the `latestTimestamp` value from the persistence repository and set it as the value of the
     * InMemory repository.
     */
    void loadLatestTimestampFromPersistentRepository();

    /**
     * This method is overridden from the `gateway::DeviceRepository` interface.
     * This method will save the device data in the cache storage, and the persistent storage if it is present.
     *
     * @param timestamp The timestamp at which the request has been sent for this device.
     * @param device The device information.
     * @return Whether the device has been successfully saved.
     */
    bool save(std::chrono::milliseconds timestamp, const RegisteredDeviceInformation& device) override;

    /**
     * This method is overridden from the `gateway::DeviceRepository` interface.
     * This method will remove the device from the cache storage, and queue the same thing to be done in persistent
     * storage if present.
     *
     * @param deviceKey The device key that needs to be removed.
     * @return Whether the device has been removed.
     */
    bool remove(const std::string& deviceKey) override;

    /**
     * This method is overridden from the `gateway::DeviceRepository` interface.
     * This method will remove all the devices from the cache storage, and queue the same thing to be done in persistent
     * storage if present.
     *
     * @return Whether the list has been cleared.
     */
    bool removeAll() override;

    /**
     * This method is overridden from the `gateway::DeviceRepository` interface.
     * This method is used to check whether data about a device key is being held. First the cache will be checked, and
     * if cache does not contain data, then persistent storage is checked (if it exists).
     *
     * @param deviceKey The device key for which presence must be confirmed.
     * @return Whether the device key is present in either cache or persistent storage.
     */
    bool containsDeviceKey(const std::string& deviceKey) override;

    /**
     * This method is overridden from the `gateway::DeviceRepository` interface.
     * This method is used to obtain the last timestamp when a device acquisition request has been sent. Devices after
     * this point have not been queried, and can be new.
     *
     * @return The last timestamp.
     */
    std::chrono::milliseconds latestTimestamp() override;

private:
    // Store the latest timestamp
    std::chrono::milliseconds m_timestamp;

    // Here we actually store the data
    std::recursive_mutex m_mutex;
    std::vector<std::string> m_devices;

    // And optional pointer for a more persistence DeviceRepository
    std::shared_ptr<DeviceRepository> m_persistentDeviceRepository;
    std::unique_ptr<CommandBuffer> m_commandBuffer;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // WOLKGATEWAY_INMEMORYDEVICEREPOSITORY_H
