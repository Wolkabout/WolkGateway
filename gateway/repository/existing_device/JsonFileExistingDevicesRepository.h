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

#ifndef JSONFILEEXISTINGDEVICESREPOSITORY_H
#define JSONFILEEXISTINGDEVICESREPOSITORY_H

#include "gateway/repository/existing_device/ExistingDevicesRepository.h"

#include <mutex>
#include <string>
#include <vector>

namespace wolkabout
{
namespace gateway
{
class JsonFileExistingDevicesRepository : public ExistingDevicesRepository
{
public:
    explicit JsonFileExistingDevicesRepository(const std::string& file = "existingDevices.json");

    void addDeviceKey(const std::string& deviceKey) override;

    std::vector<std::string> getDeviceKeys() override;

private:
    void createFileIfNotPresent();
    void readFromFile();
    void saveToFile();

    std::mutex m_fileMutex;
    const std::string m_file;
    std::vector<std::string> m_deviceKeys;
};
}    // namespace gateway
}    // namespace wolkabout

#endif
