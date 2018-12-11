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
#include "model/Message.h"
#include "protocol/GatewayFileDownloadProtocol.h"
#include "utilities/Logger.h"

#include <cassert>
#include <cmath>

namespace wolkabout
{
FileDownloadService::FileDownloadService(std::string gatewayKey, GatewayFileDownloadProtocol& protocol,
                                         uint_fast64_t maxFileSize, std::uint_fast64_t maxPacketSize,
                                         OutboundMessageHandler& outboundMessageHandler)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_maxFileSize{maxFileSize}
, m_maxPacketSize{maxPacketSize}
, m_outboundMessageHandler{outboundMessageHandler}
{
}

void FileDownloadService::download(const std::string& fileName, uint_fast64_t fileSize, const ByteArray& fileHash,
                                   const std::string& downloadDirectory, const std::string& subChannel,
                                   std::function<void(const std::string& filePath)> onSuccessCallback,
                                   std::function<void(WolkaboutFileDownloader::ErrorCode errorCode)> onFailCallback)
{
    addToCommandBuffer([=] {
        auto it = m_activeDownloads.find(subChannel);
        if (it != m_activeDownloads.end())
        {
            LOG(INFO) << "Download already active for channel: " << subChannel << ", aborting";
            it->second->abort();
        }

        LOG(INFO) << "Downloading file: " << fileName << ", on channel: " << subChannel;

        auto downloader = std::unique_ptr<FileDownloader>(new FileDownloader(m_maxFileSize, m_maxPacketSize));
        downloader->download(fileName, fileSize, fileHash, downloadDirectory,
                             [=](const FilePacketRequest& request) { requestPacket(request, subChannel); },
                             onSuccessCallback, onFailCallback);

        m_activeDownloads[subChannel] = std::move(downloader);
    });
}

void FileDownloadService::abort(const std::string& subChannel)
{
    LOG(DEBUG) << "FileDownloadService::abort " << subChannel;

    addToCommandBuffer([=] {
        auto it = m_activeDownloads.find(subChannel);
        if (it != m_activeDownloads.end())
        {
            LOG(INFO) << "Aborting file download for channel: " << subChannel;
            it->second->abort();
        }
        else
        {
            LOG(DEBUG) << "FileDownloadService::abort download not active";
        }
    });
}

void FileDownloadService::platformMessageReceived(std::shared_ptr<Message> message)
{
    assert(message);

    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(message->getChannel());
    if (deviceKey.empty())
    {
        LOG(WARN) << "Unable to extract device key from channel: " << message->getChannel();
        return;
    }

    if (m_protocol.isBinary(*message))
    {
        auto binary = m_protocol.makeBinaryData(*message);
        if (!binary)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        auto binaryData = *binary;
        addToCommandBuffer([=] { handleBinaryData(deviceKey, binaryData); });
    }
    else
    {
        LOG(WARN) << "Unable to parse message channel: " << message->getChannel();
    }
}

const GatewayProtocol& FileDownloadService::getProtocol() const
{
    return m_protocol;
}

void FileDownloadService::handleBinaryData(const std::string& deviceKey, const BinaryData& binaryData)
{
    auto it = m_activeDownloads.find(deviceKey);
    if (it == m_activeDownloads.end())
    {
        LOG(WARN) << "Unexpected binary data for device: " << deviceKey;
        return;
    }

    it->second->handleData(binaryData);
}

void FileDownloadService::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer.pushCommand(std::make_shared<std::function<void()>>(command));
}

void FileDownloadService::requestPacket(const FilePacketRequest& request, const std::string& subChannel)
{
    std::shared_ptr<Message> message = m_protocol.makeMessage(m_gatewayKey, subChannel, request);

    if (!message)
    {
        LOG(WARN) << "Failed to create file packet request";
        return;
    }

    // TODO retry handler
    m_outboundMessageHandler.addMessage(message);
}

}    // namespace wolkabout
