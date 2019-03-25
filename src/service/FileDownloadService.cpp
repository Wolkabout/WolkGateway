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
#include "OutboundMessageHandler.h"
#include "connectivity/ConnectivityService.h"
#include "model/BinaryData.h"
#include "model/FilePacketRequest.h"
#include "model/FileUploadStatus.h"
#include "model/Message.h"
#include "protocol/FileDownloadProtocol.h"
#include "repository/FileRepository.h"
#include "utilities/Logger.h"
#include <model/FileTransferStatus.h>

#include <cassert>
#include <cmath>
#include <utilities/StringUtils.h>

namespace
{
static const size_t FILE_HASH_INDEX = 0;
static const size_t FILE_DOWNLOADER_INDEX = 1;
static const size_t FLAG_INDEX = 2;
}    // namespace

namespace wolkabout
{
FileDownloadService::FileDownloadService(std::string gatewayKey, FileDownloadProtocol& protocol,
                                         std::uint64_t maxFileSize, std::uint64_t maxPacketSize,
                                         std::string fileDownloadDirectory,
                                         OutboundMessageHandler& outboundMessageHandler, FileRepository& fileRepository)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_maxFileSize{maxFileSize}
, m_maxPacketSize{maxPacketSize}
, m_fileDownloadDirectory{std::move(fileDownloadDirectory)}
, m_outboundMessageHandler{outboundMessageHandler}
, m_fileRepository{fileRepository}
, m_activeDownload{""}
, m_run{true}
, m_garbageCollector(&FileDownloadService::clearDownloads, this)
{
}

FileDownloadService::~FileDownloadService()
{
    m_run = false;
    notifyCleanup();

    if (m_garbageCollector.joinable())
    {
        m_garbageCollector.join();
    }
}

void FileDownloadService::platformMessageReceived(std::shared_ptr<Message> message)
{
    assert(message);

    if (m_protocol.isBinary(*message))
    {
        auto binary = m_protocol.makeBinaryData(*message);
        if (!binary)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        auto binaryData = *binary;
        addToCommandBuffer([=] { handleBinaryData(binaryData); });
    }
    else if (m_protocol.isUploadInitiate(*message))
    {
        auto initiate = m_protocol.makeFileUploadInitiate(*message);
        if (!initiate)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        auto initiateRequest = *initiate;
        addToCommandBuffer([=] { handleInitiateRequest(initiateRequest); });
    }
    else if (m_protocol.isUploadAbort(*message))
    {
        auto abort = m_protocol.makeFileUploadAbort(*message);
        if (!abort)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        auto abortRequest = *abort;
        addToCommandBuffer([=] { handleAbortRequest(abortRequest); });
    }
    else
    {
        LOG(WARN) << "Unable to parse message channel: " << message->getChannel();
    }
}

const Protocol& FileDownloadService::getProtocol() const
{
    return m_protocol;
}

void FileDownloadService::handleBinaryData(const BinaryData& binaryData)
{
    std::lock_guard<decltype(m_mutex)> lg{m_mutex};

    auto it = m_activeDownloads.find(m_activeDownload);
    if (it == m_activeDownloads.end())
    {
        LOG(WARN) << "Unexpected binary data";
        return;
    }

    std::get<FILE_DOWNLOADER_INDEX>(it->second)->handleData(binaryData);
}

void FileDownloadService::handleInitiateRequest(const FileUploadInitiate& request)
{
    if (request.getName().empty())
    {
        LOG(WARN) << "Missing file name from firmware update command";

        sendStatus(FileUploadStatus{request.getName(), FileTransferError::UNSPECIFIED_ERROR});
        return;
    }

    // TODO compare with maxFileSize
    if (request.getSize() == 0)
    {
        LOG(WARN) << "Missing file size from firmware update command";
        sendStatus(FileUploadStatus{request.getName(), FileTransferError::UNSPECIFIED_ERROR});
        return;
    }

    if (request.getHash().empty())
    {
        LOG(WARN) << "Missing file hash from firmware update command";
        sendStatus(FileUploadStatus{request.getName(), FileTransferError::UNSPECIFIED_ERROR});
        return;
    }

    auto fileInfo = m_fileRepository.getFileInfo(request.getName());

    if (!fileInfo)
    {
        downloadFile(request.getName(), request.getSize(), request.getHash());
    }
    else if (fileInfo->hash != request.getHash())
    {
        sendStatus(FileUploadStatus{request.getName(), FileTransferError::FILE_HASH_MISMATCH});
    }
    else
    {
        sendStatus(FileUploadStatus{request.getName(), FileTransferStatus::FILE_READY});
    }
}

void FileDownloadService::handleAbortRequest(const FileUploadAbort& request)
{
    if (request.getName().empty())
    {
        LOG(WARN) << "Missing file name from firmware update command";

        sendStatus(FileUploadStatus{request.getName(), FileTransferError::UNSPECIFIED_ERROR});
        return;
    }

    abortDownload(request.getName());
}

void FileDownloadService::downloadFile(const std::string& fileName, uint64_t fileSize, const std::string& fileHash)
{
    std::lock_guard<decltype(m_mutex)> lg{m_mutex};

    auto it = m_activeDownloads.find(fileName);
    if (it != m_activeDownloads.end())
    {
        auto activeHash = std::get<FILE_HASH_INDEX>(m_activeDownloads[fileName]);
        if (activeHash != fileHash)
        {
            LOG(WARN) << "Download already active for file: " << fileName << ", but with different hash";
            // TODO another error
            sendStatus(FileUploadStatus{fileName, FileTransferError::UNSPECIFIED_ERROR});
            return;
        }

        LOG(INFO) << "Download already active for file: " << fileName;
        sendStatus(FileUploadStatus{fileName, FileTransferStatus::FILE_TRANSFER});
        return;
    }

    LOG(INFO) << "Downloading file: " << fileName;
    sendStatus(FileUploadStatus{fileName, FileTransferStatus::FILE_TRANSFER});

    const auto byteHash = ByteUtils::toByteArray(StringUtils::base64Decode(fileHash));

    auto downloader = std::unique_ptr<FileDownloader>(new FileDownloader(m_maxFileSize, m_maxPacketSize));
    m_activeDownloads[fileName] = std::make_tuple(fileHash, std::move(downloader), false);
    m_activeDownload = fileName;

    std::get<FILE_DOWNLOADER_INDEX>(m_activeDownloads[fileName])
      ->download(fileName, fileSize, byteHash, m_fileDownloadDirectory,
                 [=](const FilePacketRequest& request) { requestPacket(request); },
                 [=](const std::string& filePath) { downloadCompleted(fileName, filePath, fileHash); },
                 [=](FileTransferError code) { downloadFailed(fileName, code); });
}

void FileDownloadService::abortDownload(const std::string& fileName)
{
    LOG(DEBUG) << "FileDownloadService::abort " << fileName;

    std::lock_guard<decltype(m_mutex)> lg{m_mutex};

    auto it = m_activeDownloads.find(fileName);
    if (it != m_activeDownloads.end())
    {
        LOG(INFO) << "Aborting download for file: " << fileName;
        std::get<FILE_DOWNLOADER_INDEX>(it->second)->abort();
        flagCompletedDownload(fileName);
        // TODO race with completed
        sendStatus(FileUploadStatus{fileName, FileTransferStatus::ABORTED});

        m_activeDownload = "";
    }
    else
    {
        LOG(DEBUG) << "FileDownloadService::abort download not active";
    }
}

void FileDownloadService::sendStatus(const FileUploadStatus& response)
{
    std::shared_ptr<Message> message = m_protocol.makeMessage(m_gatewayKey, response);

    if (!message)
    {
        LOG(ERROR) << "Failed to create file upload response";
        return;
    }

    m_outboundMessageHandler.addMessage(message);
}

void FileDownloadService::requestPacket(const FilePacketRequest& request)
{
    std::shared_ptr<Message> message = m_protocol.makeMessage(m_gatewayKey, request);

    if (!message)
    {
        LOG(WARN) << "Failed to create file packet request";
        return;
    }

    // TODO retry handler
    m_outboundMessageHandler.addMessage(message);
}

void FileDownloadService::downloadCompleted(const std::string& fileName, const std::string& filePath,
                                            const std::string& fileHash)
{
    flagCompletedDownload(fileName);

    addToCommandBuffer([=] {
        m_fileRepository.store(FileInfo{fileName, fileHash, filePath});
        sendStatus(FileUploadStatus{fileName, FileTransferStatus::FILE_READY});
    });
}

void FileDownloadService::downloadFailed(const std::string& fileName, FileTransferError errorCode)
{
    flagCompletedDownload(fileName);

    sendStatus(FileUploadStatus{fileName, errorCode});
}

void FileDownloadService::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer.pushCommand(std::make_shared<std::function<void()>>(command));
}

void FileDownloadService::flagCompletedDownload(const std::string& key)
{
    std::lock_guard<decltype(m_mutex)> lg{m_mutex};

    auto it = m_activeDownloads.find(key);
    if (it != m_activeDownloads.end())
    {
        std::get<FLAG_INDEX>(it->second) = true;
    }

    notifyCleanup();
}

void FileDownloadService::notifyCleanup()
{
    m_condition.notify_one();
}

void FileDownloadService::clearDownloads()
{
    while (m_run)
    {
        std::unique_lock<decltype(m_mutex)> lg{m_mutex};

        for (auto it = m_activeDownloads.begin(); it != m_activeDownloads.end();)
        {
            auto& tuple = it->second;
            auto& downloadCompleted = std::get<FLAG_INDEX>(tuple);

            if (downloadCompleted)
            {
                LOG(DEBUG) << "Removing completed download on channel: " << it->first;
                // removed flagged messages
                it = m_activeDownloads.erase(it);
            }
            else
            {
                ++it;
            }
        }

        lg.unlock();

        static std::mutex cvMutex;
        std::unique_lock<std::mutex> lock{cvMutex};
        m_condition.wait(lock);
    }
}
}    // namespace wolkabout
