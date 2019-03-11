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

#ifndef FILEDOWNLOADSERVICE_H
#define FILEDOWNLOADSERVICE_H

#include "GatewayInboundPlatformMessageHandler.h"
#include "WolkaboutFileDownloader.h"
#include "service/FileDownloader.h"
#include "utilities/ByteUtils.h"
#include "utilities/CommandBuffer.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <model/FileUploadAbort.h>
#include <model/FileUploadInitiate.h>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>

namespace wolkabout
{
class BinaryData;
class FileDownloadProtocol;
class FileRepository;
class FileUploadAbort;
class FileUploadInitiate;
class FileUploadStatus;
class OutboundMessageHandler;

class FileDownloadService : public PlatformMessageListener
{
public:
    FileDownloadService(std::string gatewayKey, FileDownloadProtocol& protocol, uint64_t maxFileSize,
                        std::uint64_t maxPacketSize, std::string fileDownloadDirectory,
                        OutboundMessageHandler& outboundMessageHandler, FileRepository& fileRepository);

    ~FileDownloadService();

    FileDownloadService(const FileDownloadService&) = delete;
    FileDownloadService& operator=(const FileDownloadService&) = delete;

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    const Protocol& getProtocol() const override;

private:
    void handleBinaryData(const BinaryData& binaryData);
    void handleInitiateRequest(const FileUploadInitiate& request);
    void handleAbortRequest(const FileUploadAbort& request);

    void downloadFile(const std::string& fileName, std::uint64_t fileSize, const std::string& fileHash);
    void abortDownload(const std::string& fileName);
    void sendStatus(const FileUploadStatus& response);

    void requestPacket(const FilePacketRequest& request);
    void downloadCompleted(const std::string& fileName, const std::string& filePath, const std::string& fileHash);
    void downloadFailed(const std::string& fileName, FileTransferError errorCode);

    void addToCommandBuffer(std::function<void()> command);

    void flagCompletedDownload(const std::string& key);
    void clearDownloads();
    void notifyCleanup();

    const std::string m_gatewayKey;
    const std::string m_fileDownloadDirectory;

    FileDownloadProtocol& m_protocol;

    const uint64_t m_maxFileSize;
    const uint64_t m_maxPacketSize;

    OutboundMessageHandler& m_outboundMessageHandler;
    FileRepository& m_fileRepository;

    // temporary to disallow simultaneous downloads
    std::string m_activeDownload;
    std::map<std::string, std::tuple<std::string, std::unique_ptr<FileDownloader>, bool>> m_activeDownloads;

    std::atomic_bool m_run;
    std::condition_variable m_condition;
    std::recursive_mutex m_mutex;
    std::thread m_garbageCollector;

    CommandBuffer m_commandBuffer;
};
}    // namespace wolkabout

#endif    // FILEDOWNLOADSERVICE_H
