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
#include "model/FirmwareUpdateAbort.h"
#include "model/FirmwareUpdateInstall.h"
#include "model/FirmwareUpdateStatus.h"
#include "model/FirmwareVersion.h"
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

/*** FIRMWARE UPDATE INSTALL ***/
void to_json(json& j, const FirmwareUpdateInstall& command)
{
    json commandJson;
    commandJson["fileName"] = command.getFileName();

    j = commandJson;
}
/*** FIRMWARE UPDATE INSTALL ***/

/*** FIRMWARE UPDATE STATUS ***/
static FirmwareUpdateStatus firmware_update_status_from_json(const json& j)
{
    FirmwareUpdateStatus::Status status = [&] {
        const std::string statusStr = j.at("status").get<std::string>();

        if (statusStr == "INSTALLATION")
        {
            return FirmwareUpdateStatus::Status::INSTALLATION;
        }
        else if (statusStr == "COMPLETED")
        {
            return FirmwareUpdateStatus::Status::COMPLETED;
        }
        else if (statusStr == "ABORTED")
        {
            return FirmwareUpdateStatus::Status::ABORTED;
        }
        else if (statusStr == "ERROR")
        {
            return FirmwareUpdateStatus::Status::ERROR;
        }

        throw std::logic_error("Invalid value for firmware update status");
    }();

    if (j.find("error") != j.end())
    {
        if (status != FirmwareUpdateStatus::Status::ERROR && !j["error"].is_null())
        {
            throw std::logic_error("Invalid value for firmware update error");
        }

        const int errorCode = j.at("error").get<int>();

        if (errorCode > static_cast<int>(FirmwareUpdateStatus::Error::SUBDEVICE_NOT_PRESENT))
        {
            throw std::logic_error("Invalid value for firmware update error");
        }

        auto error = static_cast<FirmwareUpdateStatus::Error>(errorCode);

        return FirmwareUpdateStatus{{}, error};
    }

    return FirmwareUpdateStatus{{}, status};
}
/*** FIRMWARE UPDATE STATUS ***/

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
    LOG(TRACE) << METHOD_INFO;

    if (command.getDeviceKeys().size() != 1)
    {
        return nullptr;
    }

    try
    {
        const std::string topic =
          FIRMWARE_UPDATE_ABORT_TOPIC_ROOT + DEVICE_PATH_PREFIX + command.getDeviceKeys().front();

        return std::unique_ptr<Message>(new Message("", topic));
    }
    catch (const std::exception& e)
    {
        LOG(DEBUG) << "Gateway firmware update protocol: Unable to serialize firmware abort command: " << e.what();
        return nullptr;
    }
    catch (...)
    {
        LOG(DEBUG) << "Gateway firmware update protocol: Unable to serialize firmware abort command";
        return nullptr;
    }
}

std::unique_ptr<Message> JsonGatewayDFUProtocol::makeMessage(const std::string& gatewayKey,
                                                             const FirmwareUpdateInstall& command) const
{
    LOG(TRACE) << METHOD_INFO;

    if (command.getDeviceKeys().size() != 1)
    {
        return nullptr;
    }

    try
    {
        const json jPayload(command);
        const std::string payload = jPayload.dump();
        const std::string topic =
          FIRMWARE_UPDATE_INSTALL_TOPIC_ROOT + DEVICE_PATH_PREFIX + command.getDeviceKeys().front();

        return std::unique_ptr<Message>(new Message(payload, topic));
    }
    catch (const std::exception& e)
    {
        LOG(DEBUG) << "Gateway firmware update protocol: Unable to serialize firmware install command: " << e.what();
        return nullptr;
    }
    catch (...)
    {
        LOG(DEBUG) << "Gateway firmware update protocol: Unable to serialize firmware install command";
        return nullptr;
    }
}

std::unique_ptr<FirmwareVersion> JsonGatewayDFUProtocol::makeFirmwareVersion(const Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    if (!StringUtils::startsWith(message.getChannel(), FIRMWARE_VERSION_TOPIC_ROOT))
    {
        return nullptr;
    }

    try
    {
        const auto key = extractDeviceKeyFromChannel(message.getChannel());
        if (key.empty())
        {
            LOG(DEBUG) << "Gateway firmware update protocol: Unable to extract device key: " << message.getChannel();
            return nullptr;
        }

        const auto& version = message.getContent();

        return std::unique_ptr<FirmwareVersion>(new FirmwareVersion(key, version));
    }
    catch (const std::exception& e)
    {
        LOG(DEBUG) << "Gateway firmware update protocol: Unable to deserialize firmware version: " << e.what();
        return nullptr;
    }
    catch (...)
    {
        LOG(DEBUG) << "Gateway firmware update protocol: Unable to deserialize firmware version";
        return nullptr;
    }
}

std::unique_ptr<FirmwareUpdateStatus> JsonGatewayDFUProtocol::makeFirmwareUpdateStatus(
  const wolkabout::Message& message) const
{
    LOG(TRACE) << METHOD_INFO;

    if (!StringUtils::startsWith(message.getChannel(), FIRMWARE_UPDATE_STATUS_TOPIC_ROOT))
    {
        return nullptr;
    }

    try
    {
        const auto key = extractDeviceKeyFromChannel(message.getChannel());
        if (key.empty())
        {
            LOG(DEBUG) << "Gateway firmware update protocol: Unable to extract device key: " << message.getChannel();
            return nullptr;
        }

        json j = json::parse(message.getContent());
        const auto status = firmware_update_status_from_json(j);

        if (status.getErrorCode())
        {
            return std::unique_ptr<FirmwareUpdateStatus>(
              new FirmwareUpdateStatus({key}, status.getErrorCode().value()));
        }
        return std::unique_ptr<FirmwareUpdateStatus>(new FirmwareUpdateStatus({key}, status.getStatus()));
    }
    catch (const std::exception& e)
    {
        LOG(DEBUG) << "Gateway firmware update protocol: Unable to deserialize firmware update status: " << e.what();
        return nullptr;
    }
    catch (...)
    {
        LOG(DEBUG) << "Gateway firmware update protocol: Unable to deserialize firmware update status";
        return nullptr;
    }
}
}    // namespace wolkabout
