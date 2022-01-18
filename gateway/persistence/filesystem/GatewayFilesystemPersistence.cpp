/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#include "gateway/persistence/filesystem/GatewayFilesystemPersistence.h"

#include "core/utilities/FileSystemUtils.h"
#include "core/utilities/Logger.h"
#include "gateway/persistence/filesystem/MessagePersister.h"

#include <regex>

namespace
{
const std::string READING_FILE_NAME = "reading_";
const std::string REGEX = READING_FILE_NAME + "(\\d+)";
}    // namespace

namespace wolkabout
{
namespace gateway
{
GatewayFilesystemPersistence::GatewayFilesystemPersistence(const std::string& persistPath, PersistenceMethod method)
: m_persister(new MessagePersister()), m_persistPath(persistPath), m_method(method), m_messageNum(0)
{
    initialize();
}

GatewayFilesystemPersistence::~GatewayFilesystemPersistence() = default;

bool GatewayFilesystemPersistence::push(std::shared_ptr<Message> message)
{
    std::lock_guard<decltype(m_mutex)> guard{m_mutex};
    return saveToDisk(message) != "";
}

void GatewayFilesystemPersistence::pop()
{
    std::lock_guard<decltype(m_mutex)> guard{m_mutex};
    if (empty())
    {
        return;
    }

    m_method == PersistenceMethod::FIFO ? deleteFirstReading() : deleteLastReading();
}

std::shared_ptr<Message> GatewayFilesystemPersistence::front()
{
    std::lock_guard<decltype(m_mutex)> guard{m_mutex};
    if (empty())
    {
        LOG(DEBUG) << "No readings to load";
        return nullptr;
    }

    const auto reading = m_method == PersistenceMethod::FIFO ? firstReading() : lastReading();
    const std::string path = readingPath(reading);
    LOG(INFO) << "Loading reading " << reading;

    std::string messageContent;

    if (!FileSystemUtils::readFileContent(path, messageContent))
    {
        LOG(ERROR) << "Failed to read readings file " << reading;

        pop();

        return nullptr;
    }

    return m_persister->load(messageContent);
}

bool GatewayFilesystemPersistence::empty() const
{
    std::lock_guard<decltype(m_mutex)> guard{m_mutex};
    return m_readingFiles.empty();
}

std::string GatewayFilesystemPersistence::saveToDisk(std::shared_ptr<Message> message)
{
    const std::string fileName = READING_FILE_NAME + std::to_string(++m_messageNum);
    const std::string path = m_persistPath + "/" + fileName;
    LOG(INFO) << "Persisting reading " << fileName;

    const auto messageContent = m_persister->save(*message);

    if (!FileSystemUtils::createFileWithContent(path, messageContent))
    {
        LOG(ERROR) << "Failed to persist reading " << fileName;
        return "";
    }

    saveReading(fileName);

    return path;
}

void GatewayFilesystemPersistence::initialize()
{
    if (FileSystemUtils::isDirectoryPresent(m_persistPath))
    {
        // read file names
        auto files = FileSystemUtils::listFiles(m_persistPath);

        // filter those that match regex
        auto itend = std::remove_if(files.begin(), files.end(),
                                    [](const std::string s) { return !std::regex_match(s, std::regex(REGEX)); });
        files.erase(itend, files.end());

        if (files.size() > 0)
        {
            LOG(INFO) << "WolkPersister: Unpersisting " << static_cast<unsigned int>(files.size()) << " readings";

            // sort filenames by message number
            std::sort(files.begin(), files.end(), [this](const std::string& s1, const std::string& s2) {
                unsigned long fileNumber1, fileNumber2;

                if (!matchFileNumber(s1, fileNumber1))
                {
                    return false;
                }
                else if (!matchFileNumber(s2, fileNumber2))
                {
                    return true;
                }

                return fileNumber1 < fileNumber2;
            });

            // get the highest number
            unsigned long fileNum;
            if (matchFileNumber(files.back(), fileNum))
            {
                m_messageNum = fileNum;
            }

            std::copy(files.begin(), files.end(), std::back_inserter(m_readingFiles));
        }
    }
    else
    {
        if (!FileSystemUtils::createDirectory(m_persistPath))
        {
            LOG(ERROR) << "Could not create persist directory: " << m_persistPath;
        }
    }
}

void GatewayFilesystemPersistence::saveReading(const std::string& fileName)
{
    m_readingFiles.push_back(fileName);
}

std::string GatewayFilesystemPersistence::readingPath(const std::string& readingFileName) const
{
    return m_persistPath + "/" + readingFileName;
}

void GatewayFilesystemPersistence::deleteFirstReading()
{
    const std::string path = readingPath(m_readingFiles.front());

    LOG(INFO) << "Deleting reading " << m_readingFiles.front();
    if (FileSystemUtils::deleteFile(path))
    {
        m_readingFiles.pop_front();

        if (m_readingFiles.size() == 0)
        {
            m_messageNum = 0;
        }
    }
    else
    {
        LOG(ERROR) << "Failed to delete readings file " << m_readingFiles.front();
    }
}

void GatewayFilesystemPersistence::deleteLastReading()
{
    const std::string path = readingPath(m_readingFiles.back());

    LOG(INFO) << "Deleting reading " << m_readingFiles.back();
    if (FileSystemUtils::deleteFile(path))
    {
        m_readingFiles.pop_back();

        if (m_readingFiles.size() == 0)
        {
            m_messageNum = 0;
        }
    }
    else
    {
        LOG(ERROR) << "Failed to delete readings file " << m_readingFiles.front();
    }
}

std::string GatewayFilesystemPersistence::firstReading()
{
    return m_readingFiles.front();
}

std::string GatewayFilesystemPersistence::lastReading()
{
    return m_readingFiles.back();
}

bool GatewayFilesystemPersistence::matchFileNumber(const std::string& fileName, unsigned long& number) const
{
    const auto fileNameLen = READING_FILE_NAME.length();

    try
    {
        const std::string num = fileName.substr(fileNameLen, fileName.length() - fileNameLen);
        number = std::stoul(num);
        return true;
    }
    catch (...)
    {
        LOG(ERROR) << "Invalid reading file name: " << fileName;
    }

    return false;
}
}    // namespace gateway
}    // namespace wolkabout
