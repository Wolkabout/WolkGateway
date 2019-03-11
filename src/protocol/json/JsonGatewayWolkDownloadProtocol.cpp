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

#include "protocol/json/JsonGatewayWolkDownloadProtocol.h"
#include "model/BinaryData.h"
#include "model/FilePacketRequest.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"

using nlohmann::json;

namespace wolkabout
{
const std::string JsonGatewayWolkDownloadProtocol::NAME = "FileDownloadProtocol";

const std::string JsonGatewayWolkDownloadProtocol::CHANNEL_DELIMITER = "/";
const std::string JsonGatewayWolkDownloadProtocol::CHANNEL_MULTI_LEVEL_WILDCARD = "#";
const std::string JsonGatewayWolkDownloadProtocol::CHANNEL_SINGLE_LEVEL_WILDCARD = "+";

const std::string JsonGatewayWolkDownloadProtocol::GATEWAY_PATH_PREFIX = "g/";
const std::string JsonGatewayWolkDownloadProtocol::DEVICE_PATH_PREFIX = "d/";
const std::string JsonGatewayWolkDownloadProtocol::DEVICE_TO_PLATFORM_DIRECTION = "d2p/";
const std::string JsonGatewayWolkDownloadProtocol::PLATFORM_TO_DEVICE_DIRECTION = "p2d/";

const std::string JsonGatewayWolkDownloadProtocol::FILE_HANDLING_STATUS_TOPIC_ROOT = "d2p/file/";

const std::string JsonGatewayWolkDownloadProtocol::BINARY_TOPIC_ROOT = "p2d/file/";

/*** FILE PACKET_REQUEST ***/
static void to_json(json& j, const FilePacketRequest& p)
{
    j = json{{"fileName", p.getFileName()}, {"chunkIndex", p.getChunkIndex()}, {"chunkSize", p.getChunkSize()}};
}
/*** FILE PACKET_REQUEST ***/

const std::string& JsonGatewayWolkDownloadProtocol::getName() const
{
    return NAME;
}

std::vector<std::string> JsonGatewayWolkDownloadProtocol::getInboundChannels() const
{
    return {};
}

std::vector<std::string> JsonGatewayWolkDownloadProtocol::getInboundChannelsForDevice(
  const std::string& /*deviceKey*/) const
{
    return {};
}

std::string JsonGatewayWolkDownloadProtocol::extractDeviceKeyFromChannel(const std::string& topic) const
{
    LOG(TRACE) << METHOD_INFO;

    std::string previousToken;
    for (std::string token : StringUtils::tokenize(topic, "/"))
    {
        if (previousToken == "d")
        {
            return token;
        }

        previousToken = token;
    }

    previousToken = "";
    for (std::string token : StringUtils::tokenize(topic, "/"))
    {
        if (previousToken == "g")
        {
            return token;
        }

        previousToken = token;
    }

    return "";
}

bool JsonGatewayWolkDownloadProtocol::isMessageToPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_TO_PLATFORM_DIRECTION);
}

bool JsonGatewayWolkDownloadProtocol::isMessageFromPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), PLATFORM_TO_DEVICE_DIRECTION);
}

bool JsonGatewayWolkDownloadProtocol::isBinary(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), BINARY_TOPIC_ROOT);
}

std::unique_ptr<BinaryData> JsonGatewayWolkDownloadProtocol::makeBinaryData(const Message& message) const
{
    try
    {
        return std::unique_ptr<BinaryData>(new BinaryData(ByteUtils::toByteArray(message.getContent())));
    }
    catch (const std::invalid_argument&)
    {
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewayWolkDownloadProtocol::makeMessage(const std::string& gatewayKey,
                                                                      const std::string& deviceKey,
                                                                      const FilePacketRequest& filePacketRequest) const
{
    const json jPayload(filePacketRequest);
    const std::string payload = jPayload.dump();
    const std::string topic = [&] {
        if (deviceKey == gatewayKey)
        {
            return FILE_HANDLING_STATUS_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
        }
        else
        {
            return FILE_HANDLING_STATUS_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                   DEVICE_PATH_PREFIX + deviceKey;
        }
    }();

    return std::unique_ptr<Message>(new Message(payload, topic));
}
}    // namespace wolkabout
