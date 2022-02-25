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

#ifndef DEVICEREPOSITORY_H
#define DEVICEREPOSITORY_H

#include "core/model/messages/RegisteredDevicesResponseMessage.h"
#include "gateway/repository/DeviceOwnership.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
namespace gateway
{
// This enum represents the data that is stored about a device.
struct StoredDeviceInformation
{
public:
    StoredDeviceInformation() : m_deviceKey{}, m_deviceBelongsTo{DeviceOwnership::Platform}, m_timestamp{} {}

    StoredDeviceInformation(std::string deviceKey, DeviceOwnership deviceBelongsTo,
                            const std::chrono::milliseconds& timestamp)
    : m_deviceKey{std::move(deviceKey)}, m_deviceBelongsTo{deviceBelongsTo}, m_timestamp{timestamp}
    {
    }

    StoredDeviceInformation(const RegisteredDeviceInformation& deviceInformation, std::chrono::milliseconds timestamp)
    : m_deviceKey{deviceInformation.deviceKey}, m_deviceBelongsTo{DeviceOwnership::Platform}, m_timestamp{timestamp}
    {
    }

    const std::string& getDeviceKey() const { return m_deviceKey; }

    DeviceOwnership getDeviceBelongsTo() const { return m_deviceBelongsTo; }

    const std::chrono::milliseconds& getTimestamp() const { return m_timestamp; }

private:
    std::string m_deviceKey;
    DeviceOwnership m_deviceBelongsTo;
    std::chrono::milliseconds m_timestamp;
};

/**
 * This interface represents an persistence entity that stores information about devices.
 */
class DeviceRepository
{
public:
    /**
     * Default virtual destructor.
     */
    virtual ~DeviceRepository() = default;

    /**
     * This is the method via which the user stores information about devices.
     *
     * @param devices The list of devices that should be added to the persistence.
     * @return Whether the devices were stored successfully. If some devices were duplicate device key, they will be
     * ignored, and the function will still return true.
     */
    virtual bool save(const std::vector<StoredDeviceInformation>& devices) = 0;

    /**
     * This is the method via which the user commands the storage to delete a device.
     *
     * @param deviceKeys The list of device keys that should be removed.
     * @return Whether the devices were removed successfully. If keys were ignored without any fault, this will still be
     * true.
     */
    virtual bool remove(const std::vector<std::string>& deviceKeys) = 0;

    /**
     * This is the method via which the user commands the storage to remove all devices.
     *
     * @return Whether all devices were removed successfully.
     */
    virtual bool removeAll() = 0;

    /**
     * This is the method via which the user asks whether storage contains information about a device.
     *
     * @param deviceKey The device key for which the user is interested about.
     * @return Whether persistence contains information about the device.
     */
    virtual bool containsDevice(const std::string& deviceKey) = 0;

    /**
     * This is the method via which the user obtains information about a device by its key.
     *
     * @param deviceKey The device key for which the user is interested about.
     * @return The information about the device retrieved from persistence. Can be empty if device does not exist.
     */
    virtual StoredDeviceInformation get(const std::string& deviceKey) = 0;

    /**
     * This is the method via which the user obtains the information about all gateway owned devices.
     *
     * @return The list of gateway owned devices.
     */
    virtual std::vector<StoredDeviceInformation> getGatewayDevices() = 0;

    /**
     * This is the method via which the user obtains the latest timestamp value that is stored in the persistence.
     *
     * @return The latest timestamp value for the platform owned devices.
     */
    virtual std::chrono::milliseconds latestPlatformTimestamp() = 0;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // DEVICEREPOSITORY_H
