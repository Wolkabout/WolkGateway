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

#ifndef GATEWAYDEVICE_H
#define GATEWAYDEVICE_H

#include "model/DetailedDevice.h"
#include "model/SubdeviceManagement.h"
#include "model/WolkOptional.h"

#include <string>

namespace wolkabout
{
class GatewayDevice : public DetailedDevice
{
public:
    GatewayDevice(std::string key, std::string password, SubdeviceManagement subdeviceManagent,
                  bool firmwareUpdateEnabled = false, bool urlDownloadEnabled = false);
    GatewayDevice(std::string key, std::string password, DeviceTemplate deviceTemplate);

    WolkOptional<SubdeviceManagement> getSubdeviceManagement() const;

private:
    static const std::string FIRMWARE_UPDATE_TYPE;

    static const std::string SUBDEVICE_MANAGEMENT_PARAMETER;
    static const std::string GATEWAY_SUBDEVICE_MANAGEMENT;
    static const std::string PLATFORM_SUBDEVICE_MANAGEMENT;

    static const std::string FIRMWARE_UPDATE_PARAMETER;
    static const std::string FILE_DOWNLOAD_PARAMETER;
    static const std::string FILE_URL_PARAMETER;
};
}    // namespace wolkabout

#endif    // GATEWAYDEVICE_H
