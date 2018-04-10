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

#include "FileDownloadService.h"
#include "FileHandler.h"
#include "OutboundServiceDataHandler.h"
#include "model/BinaryData.h"
#include "model/FilePacketRequest.h"
#include <cmath>

namespace wolkabout
{
const constexpr std::chrono::milliseconds FileDownloadService::PACKET_REQUEST_TIMEOUT;

FileDownloadService::FileDownloadService(uint_fast64_t maxFileSize, uint_fast64_t maxPacketSize,
                                         std::unique_ptr<FileHandler> fileHandler,
                                         std::shared_ptr<OutboundServiceDataHandler> outboundDataHandler)
: m_maxFileSize{maxFileSize}
, m_maxPacketSize{maxPacketSize}
, m_fileHandler{std::move(fileHandler)}
, m_outboundDataHandler{outboundDataHandler}
, m_commandBuffer{new CommandBuffer()}
, m_idleState{new FileDownloadService::IdleState(*this)}
, m_downloadState{new FileDownloadService::DownloadState(*this)}
, m_currentFileName{""}
, m_currentFileSize{0}
, m_currentPacketSize{0}
, m_currentPacketCount{0}
, m_currentPacketIndex{0}
, m_currentFileHash{}
, m_currentDownloadDirectory{""}
, m_retryCount{0}
{
    m_currentState = m_idleState.get();
}

void FileDownloadService::download(const std::string& fileName, uint_fast64_t fileSize, const ByteArray& fileHash,
                                   const std::string& downloadDirectory,
                                   std::function<void(const std::string& filePath)> onSuccessCallback,
                                   std::function<void(WolkaboutFileDownloader::Error errorCode)> onFailCallback)
{
    addToCommandBuffer([=] {
        m_currentState->download(fileName, fileSize, fileHash, downloadDirectory, onSuccessCallback, onFailCallback);
    });
}

void FileDownloadService::abort()
{
    addToCommandBuffer([=] { m_currentState->abort(); });
}

void FileDownloadService::handleBinaryData(const BinaryData& binaryData)
{
    addToCommandBuffer([=] { m_currentState->handleBinaryData(binaryData); });
}

void FileDownloadService::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(command));
}

void FileDownloadService::requestPacket(unsigned index, uint_fast64_t size)
{
    ++m_retryCount;
    m_outboundDataHandler->addFilePacketRequest(FilePacketRequest{m_currentFileName, index, size});

    m_timer.start(PACKET_REQUEST_TIMEOUT, [=] {
        abort();
        if (m_currentOnFailCallback)
        {
            m_currentOnFailCallback(WolkaboutFileDownloader::Error::UNSPECIFIED_ERROR);
        }
    });
}

void FileDownloadService::packetFailed()
{
    if (m_retryCount == FileDownloadService::MAX_RETRY_COUNT)
    {
        abort();
        if (m_currentOnFailCallback)
        {
            m_currentOnFailCallback(WolkaboutFileDownloader::Error::RETRY_COUNT_EXCEEDED);
        }
    }
    else
    {
        requestPacket(m_currentPacketIndex, m_currentPacketSize);
    }
}

void FileDownloadService::clear()
{
    m_currentState = m_idleState.get();

    m_currentFileName = "";
    m_currentFileSize = 0;
    m_currentPacketSize = 0;
    m_currentPacketCount = 0;
    m_currentPacketIndex = 0;
    m_currentFileHash = {};
    m_currentDownloadDirectory = "";
    m_currentOnSuccessCallback = nullptr;
    m_currentOnFailCallback = nullptr;
    m_retryCount = 0;
}

void FileDownloadService::IdleState::handleBinaryData(const BinaryData&) {}

void FileDownloadService::IdleState::download(
  const std::string& fileName, uint_fast64_t fileSize, const ByteArray& fileHash, const std::string& downloadDirectory,
  std::function<void(const std::string& filePath)> onSuccessCallback,
  std::function<void(WolkaboutFileDownloader::Error errorCode)> onFailCallback)
{
    if (fileSize > m_service.m_maxFileSize)
    {
        if (onFailCallback)
        {
            onFailCallback(WolkaboutFileDownloader::Error::UNSUPPORTED_FILE_SIZE);
        }
        return;
    }

    if (fileSize <= m_service.m_maxPacketSize - (2 * ByteUtils::SHA_256_HASH_BYTE_LENGTH))
    {
        m_service.m_currentPacketCount = 1;
        m_service.m_currentPacketSize = fileSize + (2 * ByteUtils::SHA_256_HASH_BYTE_LENGTH);
    }
    else
    {
        const auto count =
          static_cast<double>(fileSize) / (m_service.m_maxPacketSize - (2 * ByteUtils::SHA_256_HASH_BYTE_LENGTH));
        m_service.m_currentPacketCount = static_cast<unsigned>(std::ceil(count));
        m_service.m_currentPacketSize = m_service.m_maxPacketSize;
    }

    m_service.m_currentPacketIndex = 0;

    m_service.m_currentFileName = fileName;
    m_service.m_currentFileSize = fileSize;
    m_service.m_currentFileHash = fileHash;
    m_service.m_currentDownloadDirectory = downloadDirectory;
    m_service.m_currentOnSuccessCallback = onSuccessCallback;
    m_service.m_currentOnFailCallback = onFailCallback;

    m_service.m_fileHandler->clear();

    m_service.requestPacket(0, m_service.m_currentPacketSize);

    m_service.m_currentState = m_service.m_downloadState.get();
}

void FileDownloadService::IdleState::abort() {}

void FileDownloadService::DownloadState::handleBinaryData(const BinaryData& binaryData)
{
    m_service.m_timer.stop();

    const auto result = m_service.m_fileHandler->handleData(binaryData);
    switch (result)
    {
    case FileHandler::StatusCode::OK:
    {
        if (++m_service.m_currentPacketIndex == m_service.m_currentPacketCount)
        {
            // download complete
            const auto validationResult = m_service.m_fileHandler->validateFile(m_service.m_currentFileHash);
            switch (validationResult)
            {
            case FileHandler::StatusCode::OK:
            {
                const std::string filePath = m_service.m_currentDownloadDirectory + "/" + m_service.m_currentFileName;
                const auto saveResult = m_service.m_fileHandler->saveFile(filePath);
                switch (saveResult)
                {
                case FileHandler::StatusCode::OK:
                {
                    if (m_service.m_currentOnSuccessCallback)
                    {
                        m_service.m_currentOnSuccessCallback(filePath);
                    }

                    m_service.clear();
                    return;
                }
                case FileHandler::StatusCode::FILE_HANDLING_ERROR:
                {
                    if (m_service.m_currentOnFailCallback)
                    {
                        m_service.m_currentOnFailCallback(WolkaboutFileDownloader::Error::FILE_SYSTEM_ERROR);
                    }

                    abort();
                    m_service.clear();
                    return;
                }
                default:
                {
                    if (m_service.m_currentOnFailCallback)
                    {
                        m_service.m_currentOnFailCallback(WolkaboutFileDownloader::Error::UNSPECIFIED_ERROR);
                    }

                    abort();
                    m_service.clear();
                    return;
                }
                }
            }
            case FileHandler::StatusCode::FILE_HASH_NOT_VALID:
            {
                if (m_service.m_currentOnFailCallback)
                {
                    m_service.m_currentOnFailCallback(WolkaboutFileDownloader::Error::UNSPECIFIED_ERROR);
                }

                abort();
                m_service.clear();
                return;
            }
            default:
            {
                if (m_service.m_currentOnFailCallback)
                {
                    m_service.m_currentOnFailCallback(WolkaboutFileDownloader::Error::UNSPECIFIED_ERROR);
                }

                abort();
                m_service.clear();
                return;
            }
            }
        }
        else
        {
            m_service.m_retryCount = 0;
            m_service.requestPacket(m_service.m_currentPacketIndex, m_service.m_currentPacketSize);
        }

        break;
    }
    case FileHandler::StatusCode::PACKAGE_HASH_NOT_VALID:
    {
        m_service.packetFailed();

        break;
    }
    case FileHandler::StatusCode::PREVIOUS_PACKAGE_HASH_NOT_VALID:
    {
        m_service.packetFailed();

        break;
    }
    default:
    {
        if (m_service.m_currentOnFailCallback)
        {
            m_service.m_currentOnFailCallback(WolkaboutFileDownloader::Error::UNSPECIFIED_ERROR);
        }

        abort();
        m_service.clear();
        return;
    }
    }
}

void FileDownloadService::DownloadState::download(
  const std::string&, uint_fast64_t, const ByteArray&, const std::string&, std::function<void(const std::string&)>,
  std::function<void(WolkaboutFileDownloader::Error errorCode)> onFailCallback)
{
    onFailCallback(WolkaboutFileDownloader::Error::UNSPECIFIED_ERROR);
}

void FileDownloadService::DownloadState::abort()
{
    m_service.m_fileHandler->clear();
}
}    // namespace wolkabout
