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

#include "service/FileDownloader.h"
#include "model/BinaryData.h"
#include "model/FilePacketRequest.h"
#include "utilities/FileSystemUtils.h"
#include "utilities/Logger.h"

#include <cmath>

namespace wolkabout
{
const constexpr std::chrono::milliseconds FileDownloader::PACKET_REQUEST_TIMEOUT;

FileDownloader::FileDownloader(std::uint64_t maxPacketSize) : m_maxPacketSize{maxPacketSize} {}

void FileDownloader::download(const std::string& fileName, std::uint64_t fileSize, const ByteArray& fileHash,
                              const std::string& downloadDirectory,
                              std::function<void(const FilePacketRequest&)> packetProvider,
                              std::function<void(const std::string& filePath)> onSuccessCallback,
                              std::function<void(FileTransferError errorCode)> onFailCallback)
{
    addToCommandBuffer([=] {
        m_timer.stop();
        clear();

        if (fileSize <= m_maxPacketSize - (2 * ByteUtils::SHA_256_HASH_BYTE_LENGTH))
        {
            m_currentPacketCount = 1;
            m_currentPacketSize = fileSize + (2 * ByteUtils::SHA_256_HASH_BYTE_LENGTH);
        }
        else
        {
            const auto count =
              static_cast<double>(fileSize) / (m_maxPacketSize - (2 * ByteUtils::SHA_256_HASH_BYTE_LENGTH));
            m_currentPacketCount = static_cast<unsigned>(std::ceil(count));
            m_currentPacketSize = m_maxPacketSize;
        }

        m_currentFileName = fileName;
        m_currentFileSize = fileSize;
        m_currentFileHash = fileHash;
        m_currentDownloadDirectory = downloadDirectory;
        m_packetProvider = packetProvider;
        m_currentOnSuccessCallback = onSuccessCallback;
        m_currentOnFailCallback = onFailCallback;

        requestPacket(m_currentPacketIndex, m_currentPacketSize);
    });
}

void FileDownloader::handleData(const BinaryData& binaryData)
{
    addToCommandBuffer([=] {
        m_timer.stop();

        const auto result = m_fileHandler.handleData(binaryData);
        switch (result)
        {
        case FileHandler::StatusCode::OK:
        {
            if (++m_currentPacketIndex == m_currentPacketCount)
            {
                // download complete
                const auto validationResult = m_fileHandler.validateFile(m_currentFileHash);
                switch (validationResult)
                {
                case FileHandler::StatusCode::OK:
                {
                    const auto filePath = FileSystemUtils::composePath(m_currentFileName, m_currentDownloadDirectory);
                    const auto saveResult = m_fileHandler.saveFile(m_currentFileName, m_currentDownloadDirectory);
                    switch (saveResult)
                    {
                    case FileHandler::StatusCode::OK:
                    {
                        if (m_currentOnSuccessCallback)
                        {
                            const auto abosolutePath = FileSystemUtils::absolutePath(filePath);
                            m_currentOnSuccessCallback(abosolutePath);
                        }

                        clear();
                        return;
                    }
                    case FileHandler::StatusCode::FILE_HANDLING_ERROR:
                    {
                        if (m_currentOnFailCallback)
                        {
                            m_currentOnFailCallback(FileTransferError::FILE_SYSTEM_ERROR);
                        }

                        clear();
                        return;
                    }
                    default:
                    {
                        if (m_currentOnFailCallback)
                        {
                            m_currentOnFailCallback(FileTransferError::UNSPECIFIED_ERROR);
                        }

                        clear();
                        return;
                    }
                    }
                }
                case FileHandler::StatusCode::FILE_HASH_NOT_VALID:
                default:
                {
                    if (m_currentOnFailCallback)
                    {
                        m_currentOnFailCallback(FileTransferError::UNSPECIFIED_ERROR);
                    }

                    clear();
                    return;
                }
                }
            }
            else
            {
                m_retryCount = 0;
                requestPacket(m_currentPacketIndex, m_currentPacketSize);
            }

            break;
        }
        case FileHandler::StatusCode::PACKAGE_HASH_NOT_VALID:
        {
            packetFailed();

            break;
        }
        case FileHandler::StatusCode::PREVIOUS_PACKAGE_HASH_NOT_VALID:
        {
            packetFailed();

            break;
        }
        default:
        {
            if (m_currentOnFailCallback)
            {
                m_currentOnFailCallback(FileTransferError::UNSPECIFIED_ERROR);
            }

            clear();
            return;
        }
        }
    });
}

void FileDownloader::abort()
{
    addToCommandBuffer([=] {
        m_timer.stop();
        clear();
    });
}

void FileDownloader::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer.pushCommand(std::make_shared<std::function<void()>>(command));
}

void FileDownloader::requestPacket(unsigned index, std::uint_fast64_t size)
{
    ++m_retryCount;

    m_packetProvider(FilePacketRequest{m_currentFileName, index, size});

    m_timer.start(PACKET_REQUEST_TIMEOUT, [=] { packetFailed(); });
}

void FileDownloader::packetFailed()
{
    if (m_retryCount == FileDownloader::MAX_RETRY_COUNT)
    {
        if (m_currentOnFailCallback)
        {
            m_currentOnFailCallback(FileTransferError::RETRY_COUNT_EXCEEDED);
        }
        clear();
    }
    else
    {
        requestPacket(m_currentPacketIndex, m_currentPacketSize);
    }
}

void FileDownloader::clear()
{
    m_currentFileName = "";
    m_currentFileSize = 0;
    m_currentPacketSize = 0;

    m_currentPacketCount = 0;
    m_currentPacketIndex = 0;

    m_currentFileHash = {};
    m_currentDownloadDirectory = "";

    m_packetProvider = nullptr;
    m_currentOnSuccessCallback = nullptr;
    m_currentOnFailCallback = nullptr;

    m_retryCount = 0;
    m_fileHandler.clear();
}
}    // namespace wolkabout
