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

#include "gateway/repository/existing_device/JsonFileExistingDevicesRepository.h"

#include "core/utilities/FileSystemUtils.h"
#include "core/utilities/Logger.h"
#include "core/utilities/nlohmann/json.hpp"

#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
namespace gateway
{
JsonFileExistingDevicesRepository::JsonFileExistingDevicesRepository(const std::string& file) : m_file(std::move(file))
{
    createFileIfNotPresent();
    readFromFile();
}

void JsonFileExistingDevicesRepository::addDeviceKey(const std::string& deviceKey)
{
    LOG(DEBUG) << METHOD_INFO << " " << deviceKey;

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
    LOG(DEBUG) << METHOD_INFO;

    if (!FileSystemUtils::isFilePresent(m_file))
    {
        std::lock_guard<decltype(m_fileMutex)> l(m_fileMutex);

        nlohmann::json json;
        json["deviceKeys"] = m_deviceKeys;

        if (!FileSystemUtils::createFileWithContent(m_file, json.dump()))
        {
            LOG(ERROR) << "Failed to create " << m_file;
        }

        return;
    }
}

void JsonFileExistingDevicesRepository::readFromFile()
{
    LOG(DEBUG) << METHOD_INFO;

    std::lock_guard<decltype(m_fileMutex)> l(m_fileMutex);

    try
    {
        std::string content;
        if (!FileSystemUtils::readFileContent(m_file, content))
        {
            LOG(ERROR) << "Failed to read " << m_file;
            return;
        }

        nlohmann::json json = nlohmann::json::parse(content);

        if (json.find("deviceKeys") != json.end())
        {
            m_deviceKeys = json.at("deviceKeys").get<std::vector<std::string>>();
        }
    }
    catch (...)
    {
        LOG(ERROR) << "Failed to parse " << m_file;
    }
}

void JsonFileExistingDevicesRepository::saveToFile()
{
    LOG(DEBUG) << METHOD_INFO;

    std::lock_guard<decltype(m_fileMutex)> l(m_fileMutex);

    nlohmann::json json;
    json["deviceKeys"] = m_deviceKeys;

    if (!FileSystemUtils::createFileWithContent(m_file, json.dump()))
    {
        LOG(ERROR) << "Failed to save " << m_file;
    }
}
}    // namespace gateway
}    // namespace wolkabout
