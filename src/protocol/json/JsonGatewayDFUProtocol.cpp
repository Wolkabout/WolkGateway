/*
 * Copyright 2019 WolkAbout Technology s.r.o.
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
#include "protocol/json/Json.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include "utilities/json.hpp"

using nlohmann::json;

namespace wolkabout
{
const std::string JsonGatewayDFUProtocol::FIRMWARE_UPDATE_STATUS_TOPIC_ROOT = "d2p/firmware_update_status/";
const std::string JsonGatewayDFUProtocol::FIRMWARE_VERSION_TOPIC_ROOT = "d2p/firmware_version/";

const std::string JsonGatewayDFUProtocol::FIRMWARE_UPDATE_INSTALL_TOPIC_ROOT = "p2d/firmware_update_install/";
const std::string JsonGatewayDFUProtocol::FIRMWARE_UPDATE_ABORT_TOPIC_ROOT = "p2d/firmware_update_abort/";

///*** FIRMWARE UPDATE RESPONSE ***/
// static void from_json(const json& j, FirmwareUpdateResponse& p)
//{
//    const std::string statusStr = j.at("status").get<std::string>();
//
//    FirmwareUpdateResponse::Status status;
//    if (statusStr == "FILE_TRANSFER")
//    {
//        status = FirmwareUpdateResponse::Status::FILE_TRANSFER;
//    }
//    else if (statusStr == "FILE_READY")
//    {
//        status = FirmwareUpdateResponse::Status::FILE_READY;
//    }
//    else if (statusStr == "INSTALLATION")
//    {
//        status = FirmwareUpdateResponse::Status::INSTALLATION;
//    }
//    else if (statusStr == "COMPLETED")
//    {
//        status = FirmwareUpdateResponse::Status::COMPLETED;
//    }
//    else if (statusStr == "ABORTED")
//    {
//        status = FirmwareUpdateResponse::Status::ABORTED;
//    }
//    else
//    {
//        status = FirmwareUpdateResponse::Status::ERROR;
//    }
//
//    if (j.find("error") != j.end())
//    {
//        const int errorCode = j.at("error").get<int>();
//
//        auto error = static_cast<FirmwareUpdateResponse::ErrorCode>(errorCode);
//
//        p = FirmwareUpdateResponse{status, error};
//    }
//    else
//    {
//        p = FirmwareUpdateResponse{status};
//    }
//}
///*** FIRMWARE UPDATE RESPONSE ***/

std::vector<std::string> JsonGatewayDFUProtocol::getInboundChannels() const
{
    return {FIRMWARE_UPDATE_STATUS_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD,
            FIRMWARE_VERSION_TOPIC_ROOT + DEVICE_PATH_PREFIX + CHANNEL_MULTI_LEVEL_WILDCARD};
}

std::vector<std::string> JsonGatewayDFUProtocol::getInboundChannelsForDevice(const std::string& deviceKey) const
{
    return {FIRMWARE_UPDATE_STATUS_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey,
            FIRMWARE_VERSION_TOPIC_ROOT + DEVICE_PATH_PREFIX + deviceKey};
}

std::unique_ptr<Message> JsonGatewayDFUProtocol::makeMessage(const std::string& gatewayKey,
                                                             const FirmwareUpdateAbort& command) const
{
    if (command.getDeviceKeys().size() != 1)
    {
        return nullptr;
    }

    const json jPayload(command);
    const std::string payload = jPayload.dump();
    const std::string topic = FIRMWARE_UPDATE_ABORT_TOPIC_ROOT + DEVICE_PATH_PREFIX + command.getDeviceKeys().front();

    return std::unique_ptr<Message>(new Message(payload, topic));
}

std::unique_ptr<Message> JsonGatewayDFUProtocol::makeMessage(const std::string& gatewayKey,
                                                             const FirmwareUpdateInstall& command) const
{
    if (command.getDeviceKeys().size() != 1)
    {
        return nullptr;
    }

    const json jPayload(command);
    const std::string payload = jPayload.dump();
    const std::string topic = FIRMWARE_UPDATE_INSTALL_TOPIC_ROOT + DEVICE_PATH_PREFIX + command.getDeviceKeys().front();

    return std::unique_ptr<Message>(new Message(payload, topic));
}

std::unique_ptr<FirmwareVersion> JsonGatewayDFUProtocol::makeFirmwareVersion(const Message& message) const
{
    if (!StringUtils::startsWith(message.getChannel(), FIRMWARE_VERSION_TOPIC_ROOT))
    {
        return nullptr;
    }

    try
    {
        return std::unique_ptr<FirmwareVersion>(new FirmwareVersion(firmwareVersionFromMessage(message)));
    }
    catch (...)
    {
        return nullptr;
    }
}

std::unique_ptr<FirmwareUpdateStatus> JsonGatewayDFUProtocol::makeFirmwareUpdateStatus(
  const wolkabout::Message& message) const
{
    if (!StringUtils::startsWith(message.getChannel(), FIRMWARE_UPDATE_STATUS_TOPIC_ROOT))
    {
        return nullptr;
    }

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

FirmwareVersion JsonGatewayDFUProtocol::firmwareVersionFromMessage(const Message& message) const
{
    auto key = extractDeviceKeyFromChannel(message.getChannel());
    auto version = message.getContent();

    return FirmwareVersion{key, version};
}
}    // namespace wolkabout
