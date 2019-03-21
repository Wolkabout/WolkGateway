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

#include "service/FirmwareUpdateService.h"
#include "OutboundMessageHandler.h"
#include "model/FirmwareUpdateAbort.h"
#include "model/FirmwareUpdateInstall.h"
#include "model/FirmwareVersion.h"
#include "model/Message.h"
#include "protocol/GatewayFirmwareUpdateProtocol.h"
#include "protocol/json/JsonDFUProtocol.h"
#include "repository/FileRepository.h"
#include "service/FirmwareInstaller.h"
#include "utilities/FileSystemUtils.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"

#include <utility>

namespace wolkabout
{
FirmwareUpdateService::FirmwareUpdateService(std::string gatewayKey, JsonDFUProtocol& protocol,
                                             GatewayFirmwareUpdateProtocol& gatewayProtocol,
                                             FileRepository& fileRepository,
                                             OutboundMessageHandler& outboundPlatformMessageHandler,
                                             OutboundMessageHandler& outboundDeviceMessageHandler)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_gatewayProtocol{gatewayProtocol}
, m_fileRepository{fileRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_firmwareInstaller{nullptr}
, m_currentFirmwareVersion{""}
{
}

FirmwareUpdateService::FirmwareUpdateService(std::string gatewayKey, JsonDFUProtocol& protocol,
                                             GatewayFirmwareUpdateProtocol& gatewayProtocol,
                                             FileRepository& fileRepository,
                                             OutboundMessageHandler& outboundPlatformMessageHandler,
                                             OutboundMessageHandler& outboundDeviceMessageHandler,
                                             std::shared_ptr<FirmwareInstaller> firmwareInstaller,
                                             std::string currentFirmwareVersion)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_gatewayProtocol{gatewayProtocol}
, m_fileRepository{fileRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_firmwareInstaller{std::move(firmwareInstaller)}
, m_currentFirmwareVersion{std::move(currentFirmwareVersion)}
{
}

void FirmwareUpdateService::platformMessageReceived(std::shared_ptr<Message> message)
{
    auto installCommand = m_protocol.makeFirmwareUpdateInstall(*message);
    if (installCommand)
    {
        auto installDto = *installCommand;
        addToCommandBuffer([=] { handleFirmwareUpdateCommand(installDto); });

        return;
    }

    auto abortCommand = m_protocol.makeFirmwareUpdateAbort(*message);
    if (abortCommand)
    {
        auto abortDto = *abortCommand;
        addToCommandBuffer([=] { handleFirmwareUpdateCommand(abortDto); });

        return;
    }

    LOG(WARN) << "Unable to parse message; channel: " << message->getChannel()
              << ", content: " << message->getContent();
}

void FirmwareUpdateService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    auto statusResponse = m_gatewayProtocol.makeFirmwareUpdateStatus(*message);
    if (statusResponse)
    {
        auto statusDto = *statusResponse;
        addToCommandBuffer([=] { handleFirmwareUpdateStatus(statusDto); });

        return;
    }

    auto version = m_gatewayProtocol.makeFirmwareVersion(*message);
    if (version)
    {
        auto firmvareVersion = *version;
        addToCommandBuffer([=] { handleFirmwareVersion(firmvareVersion); });

        return;
    }

    LOG(WARN) << "Unable to parse message; channel: " << message->getChannel()
              << ", content: " << message->getContent();
}

const Protocol& FirmwareUpdateService::getProtocol() const
{
    return m_protocol;
}

const GatewayProtocol& FirmwareUpdateService::getGatewayProtocol() const
{
    return m_gatewayProtocol;
}

void FirmwareUpdateService::reportFirmwareUpdateResult()
{
    if (!m_firmwareInstaller || m_currentFirmwareVersion.empty())
    {
        return;
    }

    if (!FileSystemUtils::isFilePresent(FIRMWARE_VERSION_FILE))
    {
        return;
    }

    LOG(INFO) << "Reporting firmware update result";

    std::string firmwareVersion;
    FileSystemUtils::readFileContent(FIRMWARE_VERSION_FILE, firmwareVersion);

    StringUtils::removeTrailingWhitespace(firmwareVersion);

    if (m_currentFirmwareVersion != firmwareVersion)
    {
        LOG(INFO) << "Firmware versions differ";
        sendStatus(FirmwareUpdateStatus{{m_gatewayKey}, FirmwareUpdateStatus::Status::COMPLETED});
    }
    else
    {
        LOG(ERROR) << "Firmware versions do not differ";
        sendStatus(FirmwareUpdateStatus{{m_gatewayKey}, FirmwareUpdateStatus::Error::INSTALLATION_FAILED});
    }

    LOG(INFO) << "Deleting firmware version file";
    FileSystemUtils::deleteFile(FIRMWARE_VERSION_FILE);
}

void FirmwareUpdateService::publishFirmwareVersion()
{
    if (!m_firmwareInstaller || m_currentFirmwareVersion.empty())
    {
        return;
    }

    sendVersion(FirmwareVersion{m_gatewayKey, m_currentFirmwareVersion});
}

void FirmwareUpdateService::handleFirmwareUpdateCommand(const FirmwareUpdateInstall& command)
{
    if (command.getDeviceKeys().empty())
    {
        LOG(WARN) << "Missing device keys from firmware install command";
        sendStatus(FirmwareUpdateStatus{command.getDeviceKeys(), FirmwareUpdateStatus::Error::UNSPECIFIED_ERROR});
        return;
    }

    if (command.getFileName().empty())
    {
        LOG(WARN) << "Missing file name from firmware install command";
        sendStatus(FirmwareUpdateStatus{command.getDeviceKeys(), FirmwareUpdateStatus::Error::FILE_NOT_PRESENT});
        return;
    }

    auto fileInfo = m_fileRepository.getFileInfo(command.getFileName());

    if (!fileInfo)
    {
        LOG(WARN) << "Firmware file not present: " << command.getFileName();
        sendStatus(FirmwareUpdateStatus{command.getDeviceKeys(), FirmwareUpdateStatus::Error::FILE_NOT_PRESENT});
    }
    else
    {
        install(command.getDeviceKeys(), command.getFileName());
    }
}

void FirmwareUpdateService::handleFirmwareUpdateCommand(const FirmwareUpdateAbort& command)
{
    if (command.getDeviceKeys().empty())
    {
        LOG(WARN) << "Missing device keys from firmware abort command";
        sendStatus(FirmwareUpdateStatus{command.getDeviceKeys(), FirmwareUpdateStatus::Error::UNSPECIFIED_ERROR});
        return;
    }

    abort(command.getDeviceKeys());
}

void FirmwareUpdateService::handleFirmwareUpdateStatus(const FirmwareUpdateStatus& status)
{
    if (status.getDeviceKeys().empty())
    {
        LOG(WARN) << "No keys specified in firmware update status";
        return;
    }

    switch (status.getStatus())
    {
    case FirmwareUpdateStatus::Status::INSTALLATION:
    {
        installationInProgress(status.getDeviceKeys());
        break;
    }
    case FirmwareUpdateStatus::Status::COMPLETED:
    {
        installationCompleted(status.getDeviceKeys());
        break;
    }
    case FirmwareUpdateStatus::Status::ABORTED:
    {
        installationAborted(status.getDeviceKeys());
        break;
    }
    case FirmwareUpdateStatus::Status::ERROR:
    {
        installationFailed(status.getDeviceKeys(), status.getErrorCode());
        break;
    }
    }

    // forward status to platform
    sendStatus(status);
}

void FirmwareUpdateService::handleFirmwareVersion(const FirmwareVersion& version)
{
    if (version.getVersion().empty())
    {
        LOG(WARN) << "Empty firmware version";
        return;
    }

    if (version.getDeviceKey().empty())
    {
        LOG(WARN) << "No key specified in firmware version";
        return;
    }

    sendVersion(version);
}

void FirmwareUpdateService::install(const std::vector<std::string>& deviceKeys, const std::string& fileName)
{
    for (const auto& key : deviceKeys)
    {
        if (key == m_gatewayKey)
        {
            installGatewayFirmware(fileName);
        }
        else
        {
            auto fileInfo = m_fileRepository.getFileInfo(fileName);
            if (!fileInfo)
            {
                LOG(ERROR) << "Missing file info: " << fileName;
                return;
            }

            installDeviceFirmware(key, fileInfo.value().path);
        }
    }
}

void FirmwareUpdateService::installGatewayFirmware(const std::string& filePath)
{
    LOG(INFO) << "Handling gateway firmware install";

    if (!m_firmwareInstaller)
    {
        LOG(ERROR) << "Firmware installer not set for gateway";
        sendStatus(FirmwareUpdateStatus{{m_gatewayKey}, FirmwareUpdateStatus::Error::UNSPECIFIED_ERROR});
        return;
    }

    if (!FileSystemUtils::createFileWithContent(FIRMWARE_VERSION_FILE, m_currentFirmwareVersion))
    {
        LOG(ERROR) << "Failed to create firmware version file";
        sendStatus(FirmwareUpdateStatus{{m_gatewayKey}, FirmwareUpdateStatus::Error::FILE_SYSTEM_ERROR});
        return;
    }

    sendStatus(FirmwareUpdateStatus{{m_gatewayKey}, FirmwareUpdateStatus::Status::INSTALLATION});

    if (!m_firmwareInstaller->install(filePath))
    {
        LOG(ERROR) << "Failed to install gateway firmware";

        sendStatus(FirmwareUpdateStatus{{m_gatewayKey}, FirmwareUpdateStatus::Error::INSTALLATION_FAILED});
    }
}

void FirmwareUpdateService::installDeviceFirmware(const std::string& deviceKey, const std::string& filePath)
{
    sendCommand(FirmwareUpdateInstall{{deviceKey}, filePath});
}

void FirmwareUpdateService::installationInProgress(const std::vector<std::string>& deviceKeys)
{
    for (const auto& key : deviceKeys)
    {
        LOG(INFO) << "Firmware installation in progress for device: " << key;
    }
}

void FirmwareUpdateService::installationCompleted(const std::vector<std::string>& deviceKeys)
{
    for (const auto& key : deviceKeys)
    {
        LOG(INFO) << "Firmware installation completed for device: " << key;
    }
}

void FirmwareUpdateService::installationAborted(const std::vector<std::string>& deviceKeys)
{
    for (const auto& key : deviceKeys)
    {
        LOG(INFO) << "Firmware installation aborted for device: " << key;
    }
}

void FirmwareUpdateService::installationFailed(const std::vector<std::string>& deviceKeys,
                                               WolkOptional<FirmwareUpdateStatus::Error> errorCode)
{
    for (const auto& key : deviceKeys)
    {
        LOG(INFO) << "Firmware installation failed for device: " << key
                  << (errorCode ? std::to_string(static_cast<int>(errorCode.value())) : "");
    }
}

void FirmwareUpdateService::abort(const std::vector<std::string>& deviceKeys)
{
    for (const auto& key : deviceKeys)
    {
        if (key == m_gatewayKey)
        {
            abortGatewayFirmware();
        }
        else
        {
            abortDeviceFirmware(key);
        }
    }
}

void FirmwareUpdateService::abortGatewayFirmware()
{
    LOG(INFO) << "Not aborting gateway firmware install";
}

void FirmwareUpdateService::abortDeviceFirmware(const std::string& deviceKey)
{
    LOG(INFO) << "Handling firmware update abort for device: " << deviceKey;
    sendCommand(FirmwareUpdateAbort{{deviceKey}});
}

void FirmwareUpdateService::sendStatus(const FirmwareUpdateStatus& status)
{
    std::shared_ptr<Message> message = m_protocol.makeMessage(m_gatewayKey, status);

    if (!message)
    {
        LOG(WARN) << "Failed to create firmware update status";
        return;
    }

    m_outboundPlatformMessageHandler.addMessage(message);
}

void FirmwareUpdateService::sendVersion(const FirmwareVersion& version)
{
    std::shared_ptr<Message> message = m_protocol.makeMessage(m_gatewayKey, version);

    if (!message)
    {
        LOG(WARN) << "Failed to create firmware version";
        return;
    }

    m_outboundPlatformMessageHandler.addMessage(message);
}

void FirmwareUpdateService::sendCommand(const FirmwareUpdateInstall& command)
{
    std::shared_ptr<Message> message = m_gatewayProtocol.makeMessage(m_gatewayKey, command);

    if (!message)
    {
        LOG(ERROR) << "Failed to create firmware install command";
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(message);
}

void FirmwareUpdateService::sendCommand(const FirmwareUpdateAbort& command)
{
    std::shared_ptr<Message> message = m_gatewayProtocol.makeMessage(m_gatewayKey, command);

    if (!message)
    {
        LOG(ERROR) << "Failed to create firmware update abort command";
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(message);
}

void FirmwareUpdateService::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer.pushCommand(std::make_shared<std::function<void()>>(command));
}
//
// void FirmwareUpdateService::addDeviceUpdateStatus(const std::string& deviceKey,
//                                                  DeviceUpdateStruct::DeviceUpdateStatus status, bool autoInstall)
//{
//    m_deviceUpdateStatuses[deviceKey] = {autoInstall, status};
//}
//
// void FirmwareUpdateService::setDeviceUpdateStatus(const std::string& deviceKey,
//                                                  DeviceUpdateStruct::DeviceUpdateStatus status)
//{
//    m_deviceUpdateStatuses[deviceKey].status = status;
//}
//
// bool FirmwareUpdateService::deviceUpdateStatusExists(const std::string& deviceKey)
//{
//    return m_deviceUpdateStatuses.find(deviceKey) != m_deviceUpdateStatuses.end();
//}
//
// FirmwareUpdateService::DeviceUpdateStruct FirmwareUpdateService::getDeviceUpdateStatus(const std::string& deviceKey)
//{
//    if (deviceUpdateStatusExists(deviceKey))
//    {
//        return m_deviceUpdateStatuses.at(deviceKey);
//    }
//
//    return FirmwareUpdateService::DeviceUpdateStruct{false, DeviceUpdateStruct::DeviceUpdateStatus::UNKNOWN};
//}
//
// void FirmwareUpdateService::removeDeviceUpdateStatus(const std::string& deviceKey)
//{
//    if (deviceUpdateStatusExists(deviceKey))
//    {
//        m_deviceUpdateStatuses.erase(m_deviceUpdateStatuses.find(deviceKey));
//    }
//}
//
// void FirmwareUpdateService::addFirmwareDownloadStatus(const std::string& key, const std::string& channel,
//                                                      const std::string& uri,
//                                                      FirmwareDownloadStruct::FirmwareDownloadStatus status,
//                                                      const std::vector<std::string>& deviceKeys,
//                                                      const std::string& firmwareFile)
//{
//    m_firmwareStatuses[key] = {status, channel, uri, deviceKeys, firmwareFile};
//}
//
// FirmwareUpdateService::FirmwareDownloadStruct FirmwareUpdateService::getFirmwareDownloadStatus(const std::string&
// key)
//{
//    if (firmwareDownloadStatusExists(key))
//    {
//        return m_firmwareStatuses.at(key);
//    }
//
//    return FirmwareUpdateService::FirmwareDownloadStruct{
//      FirmwareDownloadStruct::FirmwareDownloadStatus::UNKNOWN, "", "", {}, ""};
//}
//
// void FirmwareUpdateService::setFirmwareDownloadCompletedStatus(const std::string& key, const std::string&
// firmwareFile)
//{
//    m_firmwareStatuses[key].downloadedFirmwarePath = firmwareFile;
//    m_firmwareStatuses[key].status = FirmwareDownloadStruct::FirmwareDownloadStatus::COMPLETED;
//}
//
// bool FirmwareUpdateService::firmwareDownloadStatusExists(const std::string& key)
//{
//    return m_firmwareStatuses.find(key) != m_firmwareStatuses.end();
//}
//
// bool FirmwareUpdateService::firmwareDownloadStatusExistsForDevice(const std::string& deviceKey)
//{
//    for (auto& pair : m_firmwareStatuses)
//    {
//        auto& deviceList = pair.second.devices;
//
//        auto it = std::find(deviceList.begin(), deviceList.end(), deviceKey);
//        if (it != deviceList.end())
//        {
//            return true;
//        }
//    }
//
//    return false;
//}
//
// FirmwareUpdateService::FirmwareDownloadStruct FirmwareUpdateService::getFirmwareDownloadStatusForDevice(
//  const std::string& deviceKey)
//{
//    for (auto& pair : m_firmwareStatuses)
//    {
//        auto& deviceList = pair.second.devices;
//
//        auto it = std::find(deviceList.begin(), deviceList.end(), deviceKey);
//        if (it != deviceList.end())
//        {
//            return pair.second;
//        }
//    }
//
//    return FirmwareUpdateService::FirmwareDownloadStruct{
//      FirmwareDownloadStruct::FirmwareDownloadStatus::UNKNOWN, "", "", {}, ""};
//}
//
// void FirmwareUpdateService::removeDeviceFromFirmwareStatus(const std::string& deviceKey)
//{
//    for (auto& pair : m_firmwareStatuses)
//    {
//        auto& deviceList = pair.second.devices;
//
//        auto it = std::find(deviceList.begin(), deviceList.end(), deviceKey);
//        if (it != deviceList.end())
//        {
//            deviceList.erase(it);
//        }
//    }
//}
//
// void FirmwareUpdateService::removeFirmwareStatus(const std::string& key)
//{
//    auto it = m_firmwareStatuses.find(key);
//    if (it != m_firmwareStatuses.end())
//    {
//        for (const auto& deviceKey : it->second.devices)
//        {
//            removeDeviceUpdateStatus(deviceKey);
//        }
//        m_firmwareStatuses.erase(it);
//    }
//}
//
// void FirmwareUpdateService::clearUsedFirmwareFiles()
//{
//    for (auto it = m_firmwareStatuses.begin(); it != m_firmwareStatuses.end();)
//    {
//        if (it->second.devices.empty())
//        {
//            FileSystemUtils::deleteFile(it->second.downloadedFirmwarePath);
//            it = m_firmwareStatuses.erase(it);
//        }
//        else
//        {
//            ++it;
//        }
//    }
//}
}    // namespace wolkabout
