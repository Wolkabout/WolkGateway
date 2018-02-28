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

#include "DeviceManager.h"
#include "model/Device.h"
#include "model/DeviceManifest.h"
#include "utilities/Logger.h"

namespace wolkabout
{
DeviceManager::DeviceManager(std::unique_ptr<DeviceRepository> repository,
                             std::function<void(const std::string&)> protocolRegistrator)
: m_deviceRepository{std::move(repository)}, m_protocolRegistrator{protocolRegistrator}
{
}

void DeviceManager::registerDevice(std::shared_ptr<Device> device)
{
}

std::string DeviceManager::getProtocolForDevice(const std::string& deviceKey)
{
    LOG(DEBUG) << METHOD_INFO

      auto device = m_deviceRepository->findByDeviceKey(deviceKey);

    if (!device)
    {
        LOG(DEBUG) << "Device does not exist: " << deviceKey;
        return "";
    }

    return device->getManifest().getProtocol();
}
}
