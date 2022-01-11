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

#include "service/FileDownloadService.h"

#include "FileHandler.h"
#include "FileListener.h"
#include "OutboundMessageHandler.h"
#include "core/connectivity/ConnectivityService.h"
#include "core/model/Message.h"
#include "core/model/messages/FileBinaryRequestMessage.h"
#include "core/model/messages/FileBinaryResponseMessage.h"
#include "core/model/messages/FileUploadStatusMessage.h"
#include "core/model/messages/FileUrlDownloadInitMessage.h"
#include "core/protocol/FileManagementProtocol.h"
#include "core/utilities/ByteUtils.h"
#include "core/utilities/FileSystemUtils.h"
#include "core/utilities/Logger.h"
#include "core/utilities/StringUtils.h"
#include "repository/FileRepository.h"
#include "service/UrlFileDownloader.h"

#include <cassert>
#include <cmath>

namespace
{
static const size_t FILE_HASH_INDEX = 0;
static const size_t FILE_DOWNLOADER_INDEX = 1;
static const size_t FLAG_INDEX = 2;
}    // namespace

namespace wolkabout
{
FileDownloadService::FileDownloadService(std::string gatewayKey, FileManagementProtocol& protocol,
                                         std::string fileDownloadDirectory,
                                         OutboundMessageHandler& outboundMessageHandler, FileRepository& fileRepository,
                                         std::shared_ptr<UrlFileDownloader> urlFileDownloader,
                                         std::shared_ptr<FileListener> fileListener)
: m_gatewayKey{std::move(gatewayKey)}
, m_fileDownloadDirectory{std::move(fileDownloadDirectory)}
, m_protocol{protocol}
, m_outboundMessageHandler{outboundMessageHandler}
, m_fileRepository{fileRepository}
, m_urlFileDownloader{std::move(urlFileDownloader)}
, m_fileListener{std::move(fileListener)}
, m_run{true}
, m_garbageCollector(&FileDownloadService::clearDownloads, this)
{
    // Create the working directory if it does not exist
    if (!FileSystemUtils::isDirectoryPresent(m_fileDownloadDirectory))
        FileSystemUtils::createDirectory(m_fileDownloadDirectory);

    // If we have a listener, give it the directory and lambda expression
    if (m_fileListener != nullptr)
    {
        // Pass it the absolute path for directory
        m_fileListener->receiveDirectory(getDirectory());
    }
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

std::string FileDownloadService::getDirectory() const
{
    return std::string(FileSystemUtils::absolutePath(m_fileDownloadDirectory));
}

void FileDownloadService::platformMessageReceived(std::shared_ptr<Message> message)
{
    assert(message);

    // Look for the device this message is targeting
    auto type = m_protocol.getMessageType(*message);
    auto target = m_protocol.getDeviceKey(*message);
    LOG(TRACE) << "Received message '" << toString(type) << "' for target '" << target << "'.";

    // Parse the received message based on the type
    switch (type)
    {
    case MessageType::FILE_UPLOAD_INIT:
    {
        if (auto parsedMessage = m_protocol.parseFileUploadInit(message))
        {
            auto init = *parsedMessage;
            addToCommandBuffer([=] { handle(init); });
        }
        else
        {
            LOG(ERROR) << "Failed to parse incoming 'FileUploadInitiate' message.";
        }

        return;
    }
    case MessageType::FILE_UPLOAD_ABORT:
    {
        if (auto parsedMessage = m_protocol.parseFileUploadAbort(message))
        {
            auto abort = *parsedMessage;

            addToCommandBuffer([=] { handle(abort); });
        }
        else
        {
            LOG(ERROR) << "Failed to parse 'FileUploadAbort' message.";
        }

        return;
    }
    case MessageType::FILE_BINARY_RESPONSE:
    {
        if (auto parsedMessage = m_protocol.parseFileBinaryResponse(message))
        {
            auto binary = *parsedMessage;

            addToCommandBuffer([=] { handle(binary); });
        }
        else
        {
            LOG(ERROR) << "Failed to parse 'FileBinaryResponse' message.";
        }

        return;
    }
    case MessageType::FILE_URL_DOWNLOAD_INIT:
    {
        if (auto parsedMessage = m_protocol.parseFileUrlDownloadInit(message))
        {
            auto init = *parsedMessage;

            addToCommandBuffer([=] { handle(init); });
        }
        else
        {
            LOG(ERROR) << "Failed to parse 'FileUrlDownloadInit' message.";
        }
        return;
    }
    case MessageType::FILE_URL_DOWNLOAD_ABORT:
    {
        if (auto parsedMessage = m_protocol.parseFileUrlDownloadAbort(message))
        {
            auto abort = *parsedMessage;

            addToCommandBuffer([=] { handle(abort); });
        }
        else
        {
            LOG(ERROR) << "Failed to parse 'FileUrlDownloadAbort' message.";
        }
        return;
    }
    case MessageType::FILE_LIST_REQUEST:
    {
        if (auto parsedMessage = m_protocol.parseFileListRequest(message))
        {
            auto list = *parsedMessage;

            addToCommandBuffer([=] { handle(list); });
        }
        else
        {
            LOG(ERROR) << "Failed to parse 'FileListRequest' message.";
        }
        return;
    }
    case MessageType::FILE_DELETE:
    {
        if (auto parsedMessage = m_protocol.parseFileDelete(message))
        {
            auto del = *parsedMessage;

            addToCommandBuffer([=] { handle(del); });
        }
        else
        {
            LOG(ERROR) << "Failed to parse 'FileDelete' message.";
        }
        return;
    }
    case MessageType::FILE_PURGE:
    {
        if (auto parsedMessage = m_protocol.parseFilePurge(message))
        {
            auto purge = *parsedMessage;

            addToCommandBuffer([=] { handle(purge); });
        }
        else
        {
            LOG(ERROR) << "Failed to parse 'FilePurge' message.";
        }
        return;
    }
    default:
        LOG(ERROR) << "Received a message of invalid type for this service.";
        return;
    }
}

const Protocol& FileDownloadService::getProtocol() const
{
    return m_protocol;
}

void FileDownloadService::handle(const FileBinaryResponseMessage& response)
{
    std::lock_guard<decltype(m_mutex)> lg{m_mutex};

    auto it = m_activeDownloads.find(m_activeDownload);
    if (it == m_activeDownloads.end())
    {
        LOG(WARN) << "Unexpected binary data";
        return;
    }

    std::get<FILE_DOWNLOADER_INDEX>(it->second)->handle(response);
}

void FileDownloadService::handle(const FileUploadInitiateMessage& request)
{
    if (request.getName().empty())
    {
        LOG(WARN) << "Missing file name from file upload initiate";

        sendStatus(FileUploadStatusMessage{request.getName(), FileTransferError::UNKNOWN});
        return;
    }

    if (request.getSize() == 0)
    {
        LOG(WARN) << "Missing file size from file upload initiate";
        sendStatus(FileUploadStatusMessage{request.getName(), FileTransferError::UNKNOWN});
        return;
    }

    if (request.getHash().empty())
    {
        LOG(WARN) << "Missing file hash from file upload initiate";
        sendStatus(FileUploadStatusMessage{request.getName(), FileTransferError::UNKNOWN});
        return;
    }

    if (m_fileListener != nullptr && !m_fileListener->chooseToDownload(request.getName()))
    {
        LOG(WARN) << "File listener denied the file download";
        sendStatus(FileUploadStatusMessage{request.getName(), FileTransferError::UNSUPPORTED_FILE_SIZE});
        return;
    }

    auto fileInfo = m_fileRepository.getFileInfo(request.getName());

    if (!fileInfo)
    {
        download(request.getName(), request.getSize(), request.getHash());
    }
    else if (fileInfo->hash != request.getHash())
    {
        sendStatus(FileUploadStatusMessage{request.getName(), FileTransferError::FILE_HASH_MISMATCH});
    }
    else
    {
        sendStatus(FileUploadStatusMessage{request.getName(), FileTransferStatus::FILE_READY});
    }
}

void FileDownloadService::handle(const FileUploadAbortMessage& request)
{
    if (request.getName().empty())
    {
        LOG(WARN) << "Missing file name from file upload abort";

        sendStatus(FileUploadStatusMessage{request.getName(), FileTransferError::UNKNOWN});
        return;
    }

    abortDownload(request.getName());
}

void FileDownloadService::handle(const FileDeleteMessage& request)
{
    if (request.getFiles().empty())
    {
        LOG(WARN) << "Missing file name from file delete";

        sendFileList();
        return;
    }

    deleteFiles(request.getFiles());
}

void FileDownloadService::handle(const FilePurgeMessage&)
{
    purgeFiles();
}

void FileDownloadService::handle(const FileListRequestMessage&)
{
    sendFileListUpdate();
}

void FileDownloadService::handle(const FileUrlDownloadInitMessage& request)
{
    if (!m_urlFileDownloader)
    {
        LOG(WARN) << "Url downloader not available";

        sendStatus(FileUrlDownloadStatusMessage{request.getPath(), FileTransferError::TRANSFER_PROTOCOL_DISABLED});
        return;
    }

    if (request.getPath().empty())
    {
        LOG(WARN) << "Missing file url from file url download initiate";

        sendStatus(FileUrlDownloadStatusMessage{request.getPath(), FileTransferError::UNKNOWN});
        return;
    }

    urlDownload(request.getPath());
}

void FileDownloadService::handle(const FileUrlDownloadAbortMessage& request)
{
    if (!m_urlFileDownloader)
    {
        LOG(WARN) << "Url downloader not available";

        sendStatus(FileUrlDownloadStatusMessage{request.getPath(), FileTransferError::TRANSFER_PROTOCOL_DISABLED});
        return;
    }

    if (request.getPath().empty())
    {
        LOG(WARN) << "Missing file url from file url download abort";

        sendStatus(FileUrlDownloadStatusMessage{request.getPath(), FileTransferError::UNKNOWN});
        return;
    }

    abortUrlDownload(request.getPath());
}

void FileDownloadService::download(const std::string& fileName, uint64_t fileSize, const std::string& fileHash)
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
            sendStatus(FileUploadStatusMessage{fileName, FileTransferError::UNKNOWN});
            return;
        }

        LOG(INFO) << "Download already active for file: " << fileName;
        sendStatus(FileUploadStatusMessage{fileName, FileTransferStatus::FILE_TRANSFER});
        return;
    }

    LOG(INFO) << "Downloading file: " << fileName;
    sendStatus(FileUploadStatusMessage{fileName, FileTransferStatus::FILE_TRANSFER});

    const auto byteHash = ByteUtils::toByteArray(StringUtils::base64Decode(fileHash));

    auto downloader = std::unique_ptr<FileDownloader>(new FileDownloader(MAX_PACKET_SIZE));
    m_activeDownloads[fileName] = std::make_tuple(fileHash, std::move(downloader), false);
    m_activeDownload = fileName;

    std::get<FILE_DOWNLOADER_INDEX>(m_activeDownloads[fileName])
      ->download(fileName, fileSize, byteHash, m_fileDownloadDirectory,
                 [=](const FileBinaryRequestMessage& request) { requestPacket(request); },
                 [=](const std::string& filePath) { downloadCompleted(fileName, filePath, fileHash); },
                 [=](FileTransferError code) { downloadFailed(fileName, code); });
}

void FileDownloadService::urlDownload(const std::string& fileUrl)
{
    LOG(DEBUG) << "FileDownloadService::urlDownload " << fileUrl;

    m_urlFileDownloader->download(
      fileUrl, m_fileDownloadDirectory,
      [=](const std::string& url, const std::string& fileName, const std::string& filePath) {
          urlDownloadCompleted(url, fileName, filePath);
      },
      [=](const std::string& url, FileTransferError errorCode) { urlDownloadFailed(url, errorCode); });
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
        sendStatus(FileUploadStatusMessage{fileName, FileTransferStatus::ABORTED});

        m_activeDownload = "";
    }
    else
    {
        LOG(DEBUG) << "FileDownloadService::abort download not active";
    }
}

void FileDownloadService::abortUrlDownload(const std::string& fileUrl)
{
    LOG(DEBUG) << "FileDownloadService::abortUrlDownload " << fileUrl;

    LOG(INFO) << "Aborting download for file: " << fileUrl;
    m_urlFileDownloader->abort(fileUrl);

    sendStatus(FileUrlDownloadStatusMessage{fileUrl, "", FileTransferStatus::ABORTED});
}

void FileDownloadService::deleteFiles(const std::vector<std::string>& files)
{
    LOG(DEBUG) << "FileDownloadService::deleteFiles";

    for (const auto& file : files)
    {
        deleteFile(file);
    }
}

void FileDownloadService::deleteFile(const std::string& fileName)
{
    LOG(DEBUG) << "FileDownloadService::deleteFile " << fileName;

    auto info = m_fileRepository.getFileInfo(fileName);
    if (!info)
    {
        LOG(WARN) << "File info missing for file: " << fileName << ",  can't delete";
    }

    LOG(INFO) << "Deleting file: " << fileName;

    m_fileRepository.remove(fileName);

    if (m_fileListener != nullptr)
        m_fileListener->onRemovedFile(fileName);

    sendFileList();
}

void FileDownloadService::purgeFiles()
{
    LOG(DEBUG) << "FileDownloadService::purge";

    auto fileNames = m_fileRepository.getAllFileNames();
    if (!fileNames)
    {
        LOG(ERROR) << "Failed to fetch file names";
        sendFileList();
        return;
    }

    for (const auto& name : *fileNames)
    {
        auto info = m_fileRepository.getFileInfo(name);
        if (!info)
        {
            LOG(ERROR) << "File info missing for file: " << name << ",  can't delete";
            continue;
        }

        LOG(INFO) << "Deleting file: " << name;

        m_fileRepository.remove(name);

        if (m_fileListener != nullptr)
            m_fileListener->onRemovedFile(name);
    }

    sendFileList();
}

void FileDownloadService::sendFileList()
{
    LOG(DEBUG) << "FileDownloadService::sendFileList";

    addToCommandBuffer([=] { sendFileListUpdate(); });
}

void FileDownloadService::sendStatus(const FileUploadStatusMessage& response)
{
    std::shared_ptr<Message> message = m_protocol.makeOutboundMessage(m_gatewayKey, response);

    if (!message)
    {
        LOG(ERROR) << "Failed to create file upload status";
        return;
    }

    m_outboundMessageHandler.addMessage(message);
}

void FileDownloadService::sendStatus(const FileUrlDownloadStatusMessage& response)
{
    std::shared_ptr<Message> message = m_protocol.makeOutboundMessage(m_gatewayKey, response);

    if (!message)
    {
        LOG(ERROR) << "Failed to create file url download status";
        return;
    }

    m_outboundMessageHandler.addMessage(message);
}

void FileDownloadService::sendFileListUpdate()
{
    LOG(DEBUG) << "FileDownloadService::sendFileListUpdate";

    auto fileNames = m_fileRepository.getAllFileNames();
    if (!fileNames)
    {
        LOG(ERROR) << "Failed to fetch file names";
        return;
    }

    std::vector<FileInformation> fileInfos;
    for (const auto& name : *fileNames)
    {
        auto info = m_fileRepository.getFileInfo(name);
        if (info)
        {
            fileInfos.push_back(*info);
        }
    }

    std::shared_ptr<Message> message = m_protocol.makeOutboundMessage(m_gatewayKey, FileListResponseMessage{fileInfos});

    if (!message)
    {
        LOG(ERROR) << "Failed to create file list update";
        return;
    }

    m_outboundMessageHandler.addMessage(message);
}

void FileDownloadService::requestPacket(const FileBinaryRequestMessage& request)
{
    std::shared_ptr<Message> message = m_protocol.makeOutboundMessage(m_gatewayKey, request);

    if (!message)
    {
        LOG(WARN) << "Failed to create file packet request";
        return;
    }

    m_outboundMessageHandler.addMessage(message);
}

void FileDownloadService::downloadCompleted(const std::string& fileName, const std::string& filePath,
                                            const std::string& fileHash)
{
    flagCompletedDownload(fileName);

    addToCommandBuffer([=] {
        // we don't care about size, its already stored on disk
        m_fileRepository.store(FileInformation{fileName, 0, fileHash});
        sendStatus(FileUploadStatusMessage{fileName, FileTransferStatus::FILE_READY});
        if (m_fileListener != nullptr)
            m_fileListener->onNewFile(fileName);
    });

    sendFileList();
}

void FileDownloadService::downloadFailed(const std::string& fileName, FileTransferError errorCode)
{
    flagCompletedDownload(fileName);

    sendStatus(FileUploadStatusMessage{fileName, errorCode});

    sendFileList();
}

void FileDownloadService::urlDownloadCompleted(const std::string& fileUrl, const std::string& fileName,
                                               const std::string& filePath)
{
    addToCommandBuffer([=] {
        ByteArray fileContent;
        if (!FileSystemUtils::readBinaryFileContent(filePath, fileContent))
        {
            LOG(ERROR) << "Failed to open downloaded file: " << filePath;
            FileSystemUtils::deleteFile(filePath);
            sendStatus(FileUrlDownloadStatusMessage{fileUrl, FileTransferError::FILE_SYSTEM_ERROR});
            return;
        }

        auto byteHash = ByteUtils::hashSHA256(fileContent);
        auto hashStr = StringUtils::base64Encode(byteHash);

        // we don't care about size or hash, its already stored on disk
        m_fileRepository.store(FileInformation{fileName, 0, ""});
        sendStatus(FileUrlDownloadStatusMessage{fileUrl, fileName, FileTransferStatus::FILE_READY});
        if (m_fileListener != nullptr)
            m_fileListener->onNewFile(fileName);
    });

    sendFileList();
}

void FileDownloadService::urlDownloadFailed(const std::string& fileUrl, FileTransferError errorCode)
{
    sendStatus(FileUrlDownloadStatusMessage{fileUrl, errorCode});

    sendFileList();
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
