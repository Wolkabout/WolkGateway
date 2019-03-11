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

#include "protocol/json/JsonGatewayDFUProtocol.h"
#include "model/FirmwareUpdateCommand.h"
#include "model/FirmwareUpdateResponse.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"

using nlohmann::json;

namespace wolkabout
{
const std::string JsonGatewayDFUProtocol::NAME = "DFUProtocol";

const std::string JsonGatewayDFUProtocol::CHANNEL_DELIMITER = "/";
const std::string JsonGatewayDFUProtocol::CHANNEL_MULTI_LEVEL_WILDCARD = "#";
const std::string JsonGatewayDFUProtocol::CHANNEL_SINGLE_LEVEL_WILDCARD = "+";
const std::string JsonGatewayDFUProtocol::GATEWAY_PATH_PREFIX = "g/";
const std::string JsonGatewayDFUProtocol::DEVICE_PATH_PREFIX = "d/";
const std::string JsonGatewayDFUProtocol::DEVICE_TO_PLATFORM_DIRECTION = "d2p/";
const std::string JsonGatewayDFUProtocol::PLATFORM_TO_DEVICE_DIRECTION = "p2d/";

const std::string JsonGatewayDFUProtocol::FIRMWARE_UPDATE_RESPONSE_TOPIC_ROOT = "d2p/firmware/";
const std::string JsonGatewayDFUProtocol::FIRMWARE_VERSION_TOPIC_ROOT = "d2p/firmware_version/";

const std::string JsonGatewayDFUProtocol::FIRMWARE_UPDATE_COMMAND_TOPIC_ROOT = "p2d/firmware/";

/*** FIRMWARE UPDATE RESPONSE ***/
static void to_json(json& j, const FirmwareUpdateResponse& p)
{
    const std::string status = [&]() -> std::string {
        switch (p.getStatus())
        {
        case FirmwareUpdateResponse::Status::FILE_TRANSFER:
            return "FILE_TRANSFER";
        case FirmwareUpdateResponse::Status::FILE_READY:
            return "FILE_READY";
        case FirmwareUpdateResponse::Status::INSTALLATION:
            return "INSTALLATION";
        case FirmwareUpdateResponse::Status::COMPLETED:
            return "COMPLETED";
        case FirmwareUpdateResponse::Status::ABORTED:
            return "ABORTED";
        case FirmwareUpdateResponse::Status::ERROR:
            return "ERROR";
        default:
            return "ERROR";
        }
    }();

    j = json{{"status", status}};

    if (p.getErrorCode())
    {
        auto errorCode = p.getErrorCode().value();

        j.emplace("error", static_cast<int>(errorCode));
    }
}

static void from_json(const json& j, FirmwareUpdateResponse& p)
{
    const std::string statusStr = j.at("status").get<std::string>();

    FirmwareUpdateResponse::Status status;
    if (statusStr == "FILE_TRANSFER")
    {
        status = FirmwareUpdateResponse::Status::FILE_TRANSFER;
    }
    else if (statusStr == "FILE_READY")
    {
        status = FirmwareUpdateResponse::Status::FILE_READY;
    }
    else if (statusStr == "INSTALLATION")
    {
        status = FirmwareUpdateResponse::Status::INSTALLATION;
    }
    else if (statusStr == "COMPLETED")
    {
        status = FirmwareUpdateResponse::Status::COMPLETED;
    }
    else if (statusStr == "ABORTED")
    {
        status = FirmwareUpdateResponse::Status::ABORTED;
    }
    else
    {
        status = FirmwareUpdateResponse::Status::ERROR;
    }

    if (j.find("error") != j.end())
    {
        const int errorCode = j.at("error").get<int>();

        auto error = static_cast<FirmwareUpdateResponse::ErrorCode>(errorCode);

        p = FirmwareUpdateResponse{status, error};
    }
    else
    {
        p = FirmwareUpdateResponse{status};
    }
}
/*** FIRMWARE UPDATE RESPONSE ***/

/*** FIRMWARE UPDATE COMMAND ***/
static void from_json(const json& j, FirmwareUpdateCommand& p)
{
    const std::string typeStr = j.at("command").get<std::string>();

    FirmwareUpdateCommand::Type type;
    if (typeStr == "INSTALL")
    {
        type = FirmwareUpdateCommand::Type::INSTALL;
    }
    else if (typeStr == "ABORT")
    {
        type = FirmwareUpdateCommand::Type::ABORT;
    }
    else if (typeStr == "FILE_UPLOAD")
    {
        type = FirmwareUpdateCommand::Type::FILE_UPLOAD;
    }
    else if (typeStr == "URL_DOWNLOAD")
    {
        type = FirmwareUpdateCommand::Type::URL_DOWNLOAD;
    }
    else
    {
        type = FirmwareUpdateCommand::Type::UNKNOWN;
    }

    const bool autoInstall = [&]() -> bool {
        if (j.find("autoInstall") != j.end())
        {
            return j.at("autoInstall").get<bool>();
        }

        return false;
    }();

    if (type == FirmwareUpdateCommand::Type::FILE_UPLOAD)
    {
        const std::string name = [&]() -> std::string {
            if (j.find("fileName") != j.end())
            {
                return j.at("fileName").get<std::string>();
            }

            return "";
        }();

        const uint_fast64_t size = [&]() -> uint_fast64_t {
            if (j.find("fileSize") != j.end())
            {
                return j.at("fileSize").get<uint_fast64_t>();
            }

            return 0;
        }();

        const std::string hash = [&]() -> std::string {
            if (j.find("fileHash") != j.end())
            {
                return j.at("fileHash").get<std::string>();
            }

            return "";
        }();

        p = FirmwareUpdateCommand(type, name, size, hash, autoInstall);
        return;
    }
    else if (type == FirmwareUpdateCommand::Type::URL_DOWNLOAD)
    {
        const std::string url = [&]() -> std::string {
            if (j.find("fileUrl") != j.end())
            {
                return j.at("fileUrl").get<std::string>();
            }

            return "";
        }();

        p = FirmwareUpdateCommand(type, url, autoInstall);
        return;
    }
    else
    {
        p = FirmwareUpdateCommand(type);
    }
}

static void to_json(json& j, const FirmwareUpdateCommand& p)
{
    const std::string type = [&]() -> std::string {
        switch (p.getType())
        {
        case FirmwareUpdateCommand::Type::INSTALL:
            return "INSTALL";
        case FirmwareUpdateCommand::Type::ABORT:
            return "ABORT";
        case FirmwareUpdateCommand::Type::FILE_UPLOAD:
            return "FILE_UPLOAD";
        case FirmwareUpdateCommand::Type::URL_DOWNLOAD:
            return "URL_DOWNLOAD";
        case FirmwareUpdateCommand::Type::UNKNOWN:
        default:
            return "UNKNOWN";
        }
    }();

    j = json{{"command", type}};

    if (p.getName())
    {
        j.emplace("fileName", p.getName().value());
    }

    if (p.getUrl())
    {
        j.emplace("fileUrl", p.getUrl().value());
    }

    if (p.getHash())
    {
        j.emplace("fileHash", p.getHash().value());
    }

    if (p.getSize())
    {
        j.emplace("fileSize", p.getSize().value());
    }

    if (p.getAutoInstall())
    {
        j.emplace("autoInstall", p.getAutoInstall().value());
    }
}
/*** FIRMWARE UPDATE COMMAND ***/

const std::string& JsonGatewayDFUProtocol::getName() const
{
    return NAME;
}

std::vector<std::string> JsonGatewayDFUProtocol::getInboundChannels() const
{
    return {FIRMWARE_UPDATE_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            FIRMWARE_VERSION_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewayDFUProtocol::getInboundChannelsForDevice(const std::string& deviceKey) const
{
    return {FIRMWARE_UPDATE_RESPONSE_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey,
            FIRMWARE_VERSION_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey};
}

std::string JsonGatewayDFUProtocol::extractDeviceKeyFromChannel(const std::string& topic) const
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

bool JsonGatewayDFUProtocol::isMessageToPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), DEVICE_TO_PLATFORM_DIRECTION);
}

bool JsonGatewayDFUProtocol::isMessageFromPlatform(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), PLATFORM_TO_DEVICE_DIRECTION);
}

std::unique_ptr<Message> JsonGatewayDFUProtocol::makeMessage(const std::string& gatewayKey,
                                                             const std::string& deviceKey,
                                                             const FirmwareUpdateResponse& firmwareUpdateResponse) const
{
    const json jPayload(firmwareUpdateResponse);
    const std::string payload = jPayload.dump();
    const std::string topic = [&] {
        if (deviceKey == gatewayKey)
        {
            return FIRMWARE_UPDATE_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey;
        }
        else
        {
            return FIRMWARE_UPDATE_RESPONSE_TOPIC_ROOT + GATEWAY_PATH_PREFIX + gatewayKey + CHANNEL_DELIMITER +
                   DEVICE_PATH_PREFIX + deviceKey;
        }
    }();

    return std::unique_ptr<Message>(new Message(payload, topic));
}

std::unique_ptr<Message> JsonGatewayDFUProtocol::makeMessage(const std::string& deviceKey,
                                                             const FirmwareUpdateCommand& firmwareUpdateCommand) const
{
    const json jPayload(firmwareUpdateCommand);
    const std::string payload = jPayload.dump();
    const std::string topic = FIRMWARE_UPDATE_COMMAND_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey;

    return std::unique_ptr<Message>(new Message(payload, topic));
}

std::unique_ptr<Message> JsonGatewayDFUProtocol::makeFromFirmwareVersion(const std::string& deviceKey,
                                                                         const std::string& firmwareVerion) const
{
    const std::string topic = FIRMWARE_VERSION_TOPIC_ROOT + GATEWAY_PATH_PREFIX + deviceKey;
    return std::unique_ptr<Message>(new Message(firmwareVerion, topic));
}

bool JsonGatewayDFUProtocol::isFirmwareUpdateCommandMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), FIRMWARE_UPDATE_COMMAND_TOPIC_ROOT);
}

bool JsonGatewayDFUProtocol::isFirmwareUpdateResponseMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), FIRMWARE_UPDATE_RESPONSE_TOPIC_ROOT);
}

bool JsonGatewayDFUProtocol::isFirmwareVersionMessage(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    return StringUtils::startsWith(message.getChannel(), FIRMWARE_VERSION_TOPIC_ROOT);
}

std::string JsonGatewayDFUProtocol::routeDeviceToPlatformMessage(const std::string& topic,
                                                                 const std::string& gatewayKey) const
{
    const std::string deviceTopicPart = CHANNEL_DELIMITER + DEVICE_PATH_PREFIX;
    const std::string gatewayTopicPart = CHANNEL_DELIMITER + GATEWAY_PATH_PREFIX + gatewayKey;

    const auto position = topic.find(deviceTopicPart);
    if (position != std::string::npos)
    {
        std::string routedTopic = topic;
        return routedTopic.insert(position, gatewayTopicPart);
    }

    return "";
}

std::unique_ptr<FirmwareUpdateCommand> JsonGatewayDFUProtocol::makeFirmwareUpdateCommand(const Message& message) const
{
    try
    {
        const std::string content = message.getContent();

        if (StringUtils::startsWith(content, "{"))
        {
            json j = json::parse(content);

            return std::unique_ptr<FirmwareUpdateCommand>(new FirmwareUpdateCommand(j.get<FirmwareUpdateCommand>()));
        }
        else
        {
            FirmwareUpdateCommand::Type type;
            if (content == "INSTALL")
            {
                type = FirmwareUpdateCommand::Type::INSTALL;
            }
            else if (content == "ABORT")
            {
                type = FirmwareUpdateCommand::Type::ABORT;
            }
            else if (content == "FILE_UPLOAD")
            {
                type = FirmwareUpdateCommand::Type::FILE_UPLOAD;
            }
            else if (content == "URL_DOWNLOAD")
            {
                type = FirmwareUpdateCommand::Type::URL_DOWNLOAD;
            }
            else
            {
                type = FirmwareUpdateCommand::Type::UNKNOWN;
            }

            return std::unique_ptr<FirmwareUpdateCommand>(new FirmwareUpdateCommand(type));
        }
    }
    catch (...)
    {
        return nullptr;
    }
}

std::unique_ptr<FirmwareUpdateResponse> JsonGatewayDFUProtocol::makeFirmwareUpdateResponse(const Message& message) const
{
    try
    {
        const std::string content = message.getContent();

        json j = json::parse(content);

        return std::unique_ptr<FirmwareUpdateResponse>(new FirmwareUpdateResponse(j.get<FirmwareUpdateResponse>()));
    }
    catch (...)
    {
        return nullptr;
    }
}
}    // namespace wolkabout
