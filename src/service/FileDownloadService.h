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
#include <mutex>
#include <string>
#include <thread>
#include <tuple>

namespace wolkabout
{
class BinaryData;
class GatewayFileDownloadProtocol;
class OutboundMessageHandler;

class FileDownloadService : public WolkaboutFileDownloader, public PlatformMessageListener
{
public:
    FileDownloadService(std::string gatewayKey, GatewayFileDownloadProtocol& protocol, uint_fast64_t maxFileSize,
                        std::uint_fast64_t maxPacketSize, OutboundMessageHandler& outboundMessageHandler);

    ~FileDownloadService();

    FileDownloadService(const FileDownloadService&) = delete;
    FileDownloadService& operator=(const FileDownloadService&) = delete;

    void download(const std::string& fileName, std::uint_fast64_t fileSize, const ByteArray& fileHash,
                  const std::string& downloadDirectory, const std::string& subChannel,
                  std::function<void(const std::string& filePath)> onSuccessCallback,
                  std::function<void(WolkaboutFileDownloader::ErrorCode errorCode)> onFailCallback) override;

    void abort(const std::string& subChannel) override;

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    const GatewayProtocol& getProtocol() const override;

private:
    void handleBinaryData(const std::string& deviceKey, const BinaryData& binaryData);

    void requestPacket(const FilePacketRequest& request, const std::string& subChannel);

    void addToCommandBuffer(std::function<void()> command);

    void flagCompletedDownload(const std::string& key);
    void clearDownloads();
    void notifyCleanup();

    const std::string m_gatewayKey;

    GatewayFileDownloadProtocol& m_protocol;

    const uint_fast64_t m_maxFileSize;
    const uint_fast64_t m_maxPacketSize;

    OutboundMessageHandler& m_outboundMessageHandler;

    std::map<std::string, std::tuple<std::unique_ptr<FileDownloader>, bool>> m_activeDownloads;

    std::atomic_bool m_run;
    std::condition_variable m_condition;
    std::recursive_mutex m_mutex;
    std::thread m_garbageCollector;

    CommandBuffer m_commandBuffer;
};
}    // namespace wolkabout

#endif    // FILEDOWNLOADSERVICE_H
