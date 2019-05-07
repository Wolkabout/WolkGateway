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
#ifndef FILEDOWNLOADER_H
#define FILEDOWNLOADER_H

#include "FileHandler.h"
#include "model/FileTransferStatus.h"
#include "utilities/ByteUtils.h"
#include "utilities/CommandBuffer.h"
#include "utilities/Timer.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>

namespace wolkabout
{
class BinaryData;
class FilePacketRequest;

class FileDownloader
{
public:
    FileDownloader(std::uint64_t maxPacketSize);

    void download(const std::string& fileName, std::uint64_t fileSize, const ByteArray& fileHash,
                  const std::string& downloadDirectory, std::function<void(const FilePacketRequest&)> packetProvider,
                  std::function<void(const std::string& filePath)> onSuccessCallback,
                  std::function<void(FileTransferError errorCode)> onFailCallback);

    void handleData(const BinaryData& binaryData);

    void abort();

private:
    void addToCommandBuffer(std::function<void()> command);

    void requestPacket(unsigned index, std::uint64_t size);

    void packetFailed();

    void clear();

    const std::uint64_t m_maxPacketSize;

    FileHandler m_fileHandler;

    Timer m_timer;

    std::string m_currentFileName;
    std::uint64_t m_currentFileSize;
    std::uint64_t m_currentPacketSize;
    unsigned m_currentPacketCount;
    unsigned m_currentPacketIndex;
    ByteArray m_currentFileHash;
    std::string m_currentDownloadDirectory;

    std::function<void(const FilePacketRequest&)> m_packetProvider;
    std::function<void(const std::string&)> m_currentOnSuccessCallback;
    std::function<void(FileTransferError)> m_currentOnFailCallback;

    unsigned short m_retryCount;

    CommandBuffer m_commandBuffer;

    static const unsigned short MAX_RETRY_COUNT = 3;
    static const constexpr std::chrono::milliseconds PACKET_REQUEST_TIMEOUT{6000};
};
}    // namespace wolkabout

#endif    // FILEDOWNLOADER_H
