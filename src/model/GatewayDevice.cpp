/*
 * Copyright 2019 WolkAbout Technology s.r.o.
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

#include "model/GatewayDevice.h"

#include <string>

namespace wolkabout
{
const std::string GatewayDevice::FIRMWARE_UPDATE_TYPE = "DFU";
const std::string GatewayDevice::SUBDEVICE_MANAGEMENT_PARAMETER = "subdeviceManagement";
const std::string GatewayDevice::GATEWAY_SUBDEVICE_MANAGEMENT = "GATEWAY";
const std::string GatewayDevice::PLATFORM_SUBDEVICE_MANAGEMENT = "PLATFORM";
const std::string GatewayDevice::FIRMWARE_UPDATE_PARAMETER = "supportsFirmwareUpdate";
const std::string GatewayDevice::FILE_DOWNLOAD_PARAMETER = "supportsFileDownload";
const std::string GatewayDevice::FILE_URL_PARAMETER = "supportsFileURL";

GatewayDevice::GatewayDevice(std::string key, std::string password, SubdeviceManagement subdeviceManagent,
                             bool firmwareUpdateEnabled, bool urlDownloadEnabled)
: DetailedDevice{
    "", std::move(key), std::move(password),
    DeviceTemplate{
      {},
      {},
      {},
      {},
      (firmwareUpdateEnabled ? FIRMWARE_UPDATE_TYPE : ""),
      {{SUBDEVICE_MANAGEMENT_PARAMETER,
        (static_cast<bool>(subdeviceManagent) ? GATEWAY_SUBDEVICE_MANAGEMENT : PLATFORM_SUBDEVICE_MANAGEMENT)}},
      {},
      {{FIRMWARE_UPDATE_PARAMETER, firmwareUpdateEnabled},
       {FILE_DOWNLOAD_PARAMETER, true},
       {FILE_URL_PARAMETER, urlDownloadEnabled}}}}
{
}

GatewayDevice::GatewayDevice(std::string key, std::string password, DeviceTemplate deviceTemplate)
: DetailedDevice{"", key, password, deviceTemplate}
{
}

WolkOptional<SubdeviceManagement> GatewayDevice::getSubdeviceManagement() const
{
    auto it = m_deviceTemplate.getTypeParameters().find(SUBDEVICE_MANAGEMENT_PARAMETER);
    if (it != m_deviceTemplate.getTypeParameters().end())
    {
        if (it->second == GATEWAY_SUBDEVICE_MANAGEMENT)
        {
            return WolkOptional<SubdeviceManagement>(SubdeviceManagement::GATEWAY);
        }
        else if (it->second == PLATFORM_SUBDEVICE_MANAGEMENT)
        {
            return WolkOptional<SubdeviceManagement>(SubdeviceManagement::PLATFORM);
        }
    }

    return WolkOptional<SubdeviceManagement>();
}
}    // namespace wolkabout
