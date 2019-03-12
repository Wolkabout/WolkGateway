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

#include "repository/JsonFileExistingDevicesRepository.h"
#include "utilities/FileSystemUtils.h"
#include "utilities/json.hpp"

#include <fstream>
#include <iomanip>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
JsonFileExistingDevicesRepository::JsonFileExistingDevicesRepository(const std::string& file) : m_file(std::move(file))
{
    createFileIfNotPresent();
    readFromFile();
}

void JsonFileExistingDevicesRepository::addDeviceKey(const std::string& deviceKey)
{
    if (std::find(m_deviceKeys.begin(), m_deviceKeys.end(), deviceKey) != m_deviceKeys.end())
    {
        return;
    }

    m_deviceKeys.push_back(deviceKey);
    saveToFile();
}

std::vector<std::string> JsonFileExistingDevicesRepository::getDeviceKeys()
{
    return m_deviceKeys;
}

void JsonFileExistingDevicesRepository::createFileIfNotPresent()
{
    if (!FileSystemUtils::isFilePresent(m_file))
    {
        std::lock_guard<decltype(m_fileMutex)> l(m_fileMutex);

        nlohmann::json json;
        json["deviceKeys"] = m_deviceKeys;
        FileSystemUtils::createFileWithContent(m_file, json.dump());
        return;
    }
}

void JsonFileExistingDevicesRepository::readFromFile()
{
    std::lock_guard<decltype(m_fileMutex)> l(m_fileMutex);

    std::ifstream inputFileStream(m_file);
    nlohmann::json json = nlohmann::json::parse(inputFileStream);

    if (json.find("deviceKeys") != json.end())
    {
        m_deviceKeys = json.at("deviceKeys").get<std::vector<std::string>>();
    }
}

void JsonFileExistingDevicesRepository::saveToFile()
{
    std::lock_guard<decltype(m_fileMutex)> l(m_fileMutex);

    nlohmann::json json;
    json["deviceKeys"] = m_deviceKeys;

    std::ofstream outputFileStream(m_file);
    outputFileStream << std::setw(4) << json << std::endl;
}
}    // namespace wolkabout
