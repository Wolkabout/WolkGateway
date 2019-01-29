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

#include "service/FirmwareUpdateService.h"
#include "FirmwareInstaller.h"
#include "OutboundMessageHandler.h"
#include "model/FirmwareUpdateCommand.h"
#include "model/FirmwareUpdateResponse.h"
#include "model/Message.h"
#include "protocol/GatewayFirmwareUpdateProtocol.h"
#include "utilities/FileSystemUtils.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"

#include <algorithm>

namespace wolkabout
{
FirmwareUpdateService::FirmwareUpdateService(std::string gatewayKey, GatewayFirmwareUpdateProtocol& protocol,
                                             OutboundMessageHandler& outboundPlatformMessageHandler,
                                             OutboundMessageHandler& outboundDeviceMessageHandler,
                                             WolkaboutFileDownloader& wolkaboutFileDownloader,
                                             std::string firmwareDownloadDirectory,
                                             std::shared_ptr<UrlFileDownloader> urlFileDownloader)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_wolkaboutFileDownloader{wolkaboutFileDownloader}
, m_firmwareDownloadDirectory{std::move(firmwareDownloadDirectory)}
, m_firmwareInstaller{nullptr}
, m_currentFirmwareVersion{""}
, m_urlFileDownloader{urlFileDownloader}
{
}

FirmwareUpdateService::FirmwareUpdateService(std::string gatewayKey, GatewayFirmwareUpdateProtocol& protocol,
                                             OutboundMessageHandler& outboundPlatformMessageHandler,
                                             OutboundMessageHandler& outboundDeviceMessageHandler,
                                             WolkaboutFileDownloader& wolkaboutFileDownloader,
                                             std::string firmwareDownloadDirectory,
                                             std::shared_ptr<FirmwareInstaller> firmwareInstaller,
                                             std::string currentFirmwareVersion,
                                             std::shared_ptr<UrlFileDownloader> urlFileDownloader)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_wolkaboutFileDownloader{wolkaboutFileDownloader}
, m_firmwareDownloadDirectory{std::move(firmwareDownloadDirectory)}
, m_firmwareInstaller{firmwareInstaller}
, m_currentFirmwareVersion{currentFirmwareVersion}
, m_urlFileDownloader{urlFileDownloader}
{
}

void FirmwareUpdateService::platformMessageReceived(std::shared_ptr<Message> message)
{
    if (!m_protocol.isMessageFromPlatform(*message))
    {
        LOG(WARN) << "DataService: Ignoring message on channel '" << message->getChannel()
                  << "'. Message not from platform.";
        return;
    }

    if (!m_protocol.isFirmwareUpdateCommandMessage(*message))
    {
        LOG(WARN) << "Unable to parse message channel: " << message->getChannel();
        return;
    }

    auto command = m_protocol.makeFirmwareUpdateCommand(*message);
    if (!command)
    {
        LOG(WARN) << "Unable to parse message contents: " << message->getContent();
        return;
    }

    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(message->getChannel());

    auto dfuCommand = *command;
    addToCommandBuffer([=] { handleFirmwareUpdateCommand(dfuCommand, deviceKey); });
}

void FirmwareUpdateService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    if (!m_protocol.isMessageToPlatform(*message))
    {
        LOG(WARN) << "DataService: Ignoring message on channel '" << message->getChannel()
                  << "'. Message not from device.";
        return;
    }

    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(message->getChannel());

    if (m_protocol.isFirmwareUpdateResponseMessage(*message))
    {
        auto response = m_protocol.makeFirmwareUpdateResponse(*message);
        if (!response)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        auto dfuResponse = *response;
        addToCommandBuffer([=] { handleFirmwareUpdateResponse(dfuResponse, deviceKey); });
    }
    else if (m_protocol.isFirmwareVersionMessage(*message))
    {
        routeDeviceToPlatformMessage(message);
    }
    else
    {
        LOG(WARN) << "Unable to parse message channel: " << message->getChannel();
        return;
    }
}

const GatewayProtocol& FirmwareUpdateService::getProtocol() const
{
    return m_protocol;
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

    std::string firmwareVersion;
    FileSystemUtils::readFileContent(FIRMWARE_VERSION_FILE, firmwareVersion);

    StringUtils::removeTrailingWhitespace(firmwareVersion);

    if (m_currentFirmwareVersion != firmwareVersion)
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::COMPLETED}, m_gatewayKey);
    }
    else
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::INSTALLATION_FAILED},
                     m_gatewayKey);
    }

    FileSystemUtils::deleteFile(FIRMWARE_VERSION_FILE);
}

void FirmwareUpdateService::publishFirmwareVersion()
{
    if (!m_firmwareInstaller || m_currentFirmwareVersion.empty())
    {
        return;
    }

    const std::shared_ptr<Message> message = m_protocol.makeFromFirmwareVersion(m_gatewayKey, m_currentFirmwareVersion);

    if (!message)
    {
        LOG(WARN) << "Failed to create firmware version message";
        return;
    }

    m_outboundPlatformMessageHandler.addMessage(message);
}

void FirmwareUpdateService::handleFirmwareUpdateCommand(const FirmwareUpdateCommand& command,
                                                        const std::string deviceKey)
{
    std::vector<std::string> deviceKeys;
    std::string subChannel;

    if (deviceKey.empty())    // update multiple devices
    {
        // TODO extract keys from command
        // TODO set subChannel
        LOG(ERROR) << "Unable to extract device keys from firmware update command";
        return;
    }
    else    // update single device
    {
        deviceKeys.push_back(deviceKey);
        subChannel = deviceKey;
    }

    switch (command.getType())
    {
    case FirmwareUpdateCommand::Type::FILE_UPLOAD:
    {
        if (!command.getName() || command.getName().value().empty())
        {
            LOG(WARN) << "Missing file name from firmware update command";

            sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                                FirmwareUpdateResponse::ErrorCode::UNSPECIFIED_ERROR},
                         deviceKey);
            return;
        }

        // TODO compare with maxFileSize
        if (!command.getSize() || command.getSize().value() == 0)
        {
            LOG(WARN) << "Missing file size from firmware update command";
            sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                                FirmwareUpdateResponse::ErrorCode::UNSPECIFIED_ERROR},
                         deviceKey);
            return;
        }

        if (!command.getHash() || command.getHash().value().empty())
        {
            LOG(WARN) << "Missing file hash from firmware update command";
            sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                                FirmwareUpdateResponse::ErrorCode::UNSPECIFIED_ERROR},
                         deviceKey);
            return;
        }

        bool autoInstall = command.getAutoInstall() ? command.getAutoInstall().value() : false;

        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::FILE_TRANSFER}, subChannel);

        for (const auto& key : deviceKeys)
        {
            fileUpload(key, command.getName().value(), command.getSize().value(), command.getHash().value(),
                       autoInstall, subChannel);
        }

        break;
    }
    case FirmwareUpdateCommand::Type::URL_DOWNLOAD:
    {
        if (!m_urlFileDownloader)
        {
            sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                                FirmwareUpdateResponse::ErrorCode::FILE_UPLOAD_DISABLED},
                         m_gatewayKey);
            return;
        }

        if (!command.getUrl() || command.getUrl().value().empty())
        {
            LOG(WARN) << "Missing url from firmware update command";
            sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                                FirmwareUpdateResponse::ErrorCode::UNSPECIFIED_ERROR},
                         deviceKey);
            return;
        }

        bool autoInstall = command.getAutoInstall() ? false : command.getAutoInstall().value();

        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::FILE_TRANSFER}, subChannel);

        for (const auto& key : deviceKeys)
        {
            urlDownload(key, command.getUrl().value(), autoInstall, subChannel);
        }

        break;
    }
    case FirmwareUpdateCommand::Type::INSTALL:
    {
        install(deviceKey);
        break;
    }
    case FirmwareUpdateCommand::Type::ABORT:
    {
        abort(deviceKey);
        break;
    }
    default:
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::UNSPECIFIED_ERROR},
                     deviceKey);
    }
    }
}

void FirmwareUpdateService::handleFirmwareUpdateResponse(const FirmwareUpdateResponse& response,
                                                         const std::string deviceKey)
{
    if (!deviceUpdateStatusExists(deviceKey))
    {
        LOG(ERROR) << "Missing firmware update info for device: " << deviceKey;
    }

    switch (response.getStatus())
    {
    case FirmwareUpdateResponse::Status::FILE_TRANSFER:
    {
        LOG(INFO) << "Firmware file transfer in progress for device: " << deviceKey;
        setDeviceUpdateStatus(deviceKey, DeviceUpdateStruct::DeviceUpdateStatus::TRANSFER);
        break;
    }
    case FirmwareUpdateResponse::Status::FILE_READY:
    {
        LOG(INFO) << "Firmware file ready for device: " << deviceKey;
        auto deviceUpdateStatus = getDeviceUpdateStatus(deviceKey);

        setDeviceUpdateStatus(deviceKey, DeviceUpdateStruct::DeviceUpdateStatus::READY);
        break;
    }
    case FirmwareUpdateResponse::Status::INSTALLATION:
    {
        LOG(INFO) << "Firmware installation in progress for device: " << deviceKey;
        setDeviceUpdateStatus(deviceKey, DeviceUpdateStruct::DeviceUpdateStatus::INSTALL);
        break;
    }
    case FirmwareUpdateResponse::Status::COMPLETED:
    {
        LOG(INFO) << "Firmware update completed for device: " << deviceKey;
        installCompleted(deviceKey);
        break;
    }
    case FirmwareUpdateResponse::Status::ABORTED:
    {
        LOG(INFO) << "Firmware update aborted for device: " << deviceKey;
        installAborted(deviceKey);
        break;
    }
    case FirmwareUpdateResponse::Status::ERROR:
    {
        LOG(INFO) << "Firmware update error for device: " << deviceKey;
        installFailed(deviceKey);
        break;
    }
    }

    sendResponse(response, deviceKey);
}

void FirmwareUpdateService::routeDeviceToPlatformMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = m_protocol.routeDeviceToPlatformMessage(message->getChannel(), m_gatewayKey);
    if (channel.empty())
    {
        LOG(WARN) << "Failed to route device message: " << message->getChannel();
        return;
    }

    const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), channel)};
    m_outboundPlatformMessageHandler.addMessage(routedMessage);
}

void FirmwareUpdateService::fileUpload(const std::string& deviceKey, const std::string& name, std::uint_fast64_t size,
                                       const std::string& hash, bool autoInstall, const std::string& subChannel)
{
    if (deviceUpdateStatusExists(deviceKey))
    {
        removeDeviceFromFirmwareStatus(deviceKey);
    }

    addDeviceUpdateStatus(deviceKey, DeviceUpdateStruct::DeviceUpdateStatus::WOLK_DOWNLOAD, autoInstall);

    if (!firmwareDownloadStatusExists(hash))
    {
        addFirmwareDownloadStatus(hash, subChannel, name, FirmwareDownloadStruct::FirmwareDownloadStatus::IN_PROGRESS,
                                  {deviceKey});

        const auto byteHash = ByteUtils::toByteArray(StringUtils::base64Decode(hash));

        m_wolkaboutFileDownloader.download(
          name, size, byteHash, m_firmwareDownloadDirectory, subChannel,
          [=](const std::string& filePath) { downloadCompleted(filePath, hash, subChannel); },
          [=](WolkaboutFileDownloader::ErrorCode errorCode) { downloadFailed(errorCode, hash, subChannel); });
    }
    else
    {
        m_firmwareStatuses[hash].devices.push_back(deviceKey);
        if (m_firmwareStatuses[hash].status == FirmwareDownloadStruct::FirmwareDownloadStatus::COMPLETED)
        {
            const auto firmwareFile = m_firmwareStatuses[hash].downloadedFirmwarePath;
            transferFile(deviceKey, firmwareFile);
        }
    }
}

void FirmwareUpdateService::urlDownload(const std::string& deviceKey, const std::string& url, bool autoInstall,
                                        const std::string& subChannel)
{
    if (deviceUpdateStatusExists(deviceKey))
    {
        removeDeviceFromFirmwareStatus(deviceKey);
    }

    addDeviceUpdateStatus(deviceKey, DeviceUpdateStruct::DeviceUpdateStatus::URL_DOWNLOAD, autoInstall);

    if (!firmwareDownloadStatusExists(url))
    {
        addFirmwareDownloadStatus(url, subChannel, url, FirmwareDownloadStruct::FirmwareDownloadStatus::IN_PROGRESS,
                                  {deviceKey});

        const auto byteHash = ByteUtils::toByteArray(StringUtils::base64Decode(url));

        m_urlFileDownloader->download(url, m_firmwareDownloadDirectory,
                                      [=](const std::string& callbackUrl, const std::string& filePath) {
                                          downloadCompleted(filePath, callbackUrl, subChannel);
                                      },
                                      [=](const std::string& callbackUrl, UrlFileDownloader::Error errorCode) {
                                          downloadFailed(errorCode, callbackUrl, subChannel);
                                      });
    }
    else
    {
        m_firmwareStatuses[url].devices.push_back(deviceKey);
        if (m_firmwareStatuses[url].status == FirmwareDownloadStruct::FirmwareDownloadStatus::COMPLETED)
        {
            const auto firmwareFile = m_firmwareStatuses[url].downloadedFirmwarePath;
            transferFile(deviceKey, firmwareFile);
        }
    }
}

void FirmwareUpdateService::downloadCompleted(const std::string& filePath, const std::string& hash,
                                              const std::string& subChannel)
{
    addToCommandBuffer([=] {
        if (!firmwareDownloadStatusExists(hash))
        {
            LOG(ERROR) << "Missing device info for downloaded firmware file: " << filePath << ", on channel "
                       << subChannel;
        }

        setFirmwareDownloadCompletedStatus(hash, filePath);

        for (const std::string& deviceKey : getFirmwareDownloadStatus(hash).devices)
        {
            if (!deviceUpdateStatusExists(deviceKey))
            {
                LOG(ERROR) << "Missing firmware update info for device: " << deviceKey;
                continue;
            }

            if (deviceKey == m_gatewayKey)
            {
                sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::FILE_READY}, m_gatewayKey);

                if (getDeviceUpdateStatus(m_gatewayKey).autoinstall)
                {
                    install(m_gatewayKey);
                }
            }
            else
            {
                transferFile(deviceKey, filePath);
            }
        }

        // TODO update download status
    });
}

void FirmwareUpdateService::downloadFailed(WolkaboutFileDownloader::ErrorCode errorCode, const std::string& hash,
                                           const std::string& subChannel)
{
    switch (errorCode)
    {
    case WolkaboutFileDownloader::ErrorCode::FILE_SYSTEM_ERROR:
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::FILE_SYSTEM_ERROR},
                     subChannel);
        break;
    }
    case WolkaboutFileDownloader::ErrorCode::RETRY_COUNT_EXCEEDED:
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::RETRY_COUNT_EXCEEDED},
                     subChannel);
        break;
    }
    case WolkaboutFileDownloader::ErrorCode::UNSUPPORTED_FILE_SIZE:
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::UNSUPPORTED_FILE_SIZE},
                     subChannel);
        break;
    }
    case WolkaboutFileDownloader::ErrorCode::UNSPECIFIED_ERROR:
    default:
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::UNSPECIFIED_ERROR},
                     subChannel);
        break;
    }
    }

    removeFirmwareStatus(hash);
}

void FirmwareUpdateService::downloadFailed(UrlFileDownloader::Error errorCode, const std::string& url,
                                           const std::string& subChannel)
{
    switch (errorCode)
    {
    case UrlFileDownloader::Error::MALFORMED_URL:
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::MALFORMED_URL},
                     subChannel);
        break;
    }
    case UrlFileDownloader::Error::FILE_SYSTEM_ERROR:
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::FILE_SYSTEM_ERROR},
                     subChannel);
        break;
    }
    case UrlFileDownloader::Error::UNSUPPORTED_FILE_SIZE:
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::UNSUPPORTED_FILE_SIZE},
                     subChannel);
        break;
    }
    case UrlFileDownloader::Error::UNSPECIFIED_ERROR:
    default:
    {
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::UNSPECIFIED_ERROR},
                     subChannel);
        break;
    }
    }

    removeFirmwareStatus(url);
}

void FirmwareUpdateService::transferFile(const std::string& deviceKey, const std::string& filePath)
{
    if (!deviceUpdateStatusExists(deviceKey))
    {
        LOG(ERROR) << "Missing firmware update info for device: " << deviceKey;
        return;
    }

    auto status = getDeviceUpdateStatus(deviceKey);

    setDeviceUpdateStatus(deviceKey, DeviceUpdateStruct::DeviceUpdateStatus::TRANSFER);

    auto fullPath = FileSystemUtils::absolutePath(filePath);

    FirmwareUpdateCommand command{FirmwareUpdateCommand::Type::URL_DOWNLOAD, fullPath, status.autoinstall};

    sendCommand(command, deviceKey);
}

void FirmwareUpdateService::install(const std::string& deviceKey)
{
    if (deviceKey == m_gatewayKey)
    {
        if (!deviceUpdateStatusExists(m_gatewayKey))
        {
            LOG(ERROR) << "Missing firmware update info for gateway";
            sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                                FirmwareUpdateResponse::ErrorCode::UNSPECIFIED_ERROR},
                         m_gatewayKey);
        }

        auto fullPath =
          FileSystemUtils::absolutePath(getFirmwareDownloadStatusForDevice(m_gatewayKey).downloadedFirmwarePath);

        installGwFirmware(fullPath);
    }
    else
    {
        FirmwareUpdateCommand command{FirmwareUpdateCommand::Type::INSTALL};

        sendCommand(command, deviceKey);
    }
}

void FirmwareUpdateService::installGwFirmware(const std::string& filePath)
{
    LOG(INFO) << "Gateway firmware install";

    if (!m_firmwareInstaller)
    {
        LOG(ERROR) << "Firmware installer not set for gateway";
        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::UNSPECIFIED_ERROR},
                     m_gatewayKey);
        return;
    }

    sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::INSTALLATION}, m_gatewayKey);

    if (!FileSystemUtils::createFileWithContent(FIRMWARE_VERSION_FILE, m_currentFirmwareVersion))
    {
        removeDeviceUpdateStatus(m_gatewayKey);

        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::INSTALLATION_FAILED},
                     m_gatewayKey);
        return;
    }

    if (!m_firmwareInstaller->install(filePath))
    {
        removeDeviceUpdateStatus(m_gatewayKey);

        sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ERROR,
                                            FirmwareUpdateResponse::ErrorCode::INSTALLATION_FAILED},
                     m_gatewayKey);
    }
}

void FirmwareUpdateService::installCompleted(const std::string& deviceKey)
{
    removeDeviceUpdateStatus(deviceKey);
    removeDeviceFromFirmwareStatus(deviceKey);
    clearUsedFirmwareFiles();
}

void FirmwareUpdateService::installAborted(const std::string& deviceKey)
{
    removeDeviceUpdateStatus(deviceKey);
    removeDeviceFromFirmwareStatus(deviceKey);
    clearUsedFirmwareFiles();
}

void FirmwareUpdateService::installFailed(const std::string& deviceKey)
{
    removeDeviceUpdateStatus(deviceKey);
    removeDeviceFromFirmwareStatus(deviceKey);
    clearUsedFirmwareFiles();
}

void FirmwareUpdateService::abort(const std::string& deviceKey)
{
    if (!deviceUpdateStatusExists(deviceKey))
    {
        LOG(ERROR) << "Missing firmware update info for device: " << deviceKey;
        return;
    }

    auto status = getDeviceUpdateStatus(deviceKey);

    // if download is active just abort download
    if (status.status == DeviceUpdateStruct::DeviceUpdateStatus::WOLK_DOWNLOAD)
    {
        if (firmwareDownloadStatusExistsForDevice(deviceKey))
        {
            auto firmwareStatus = getFirmwareDownloadStatusForDevice(deviceKey);
            m_wolkaboutFileDownloader.abort(firmwareStatus.channel);
            sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ABORTED}, firmwareStatus.channel);

            installAborted(deviceKey);
        }

        return;
    }
    else if (status.status == DeviceUpdateStruct::DeviceUpdateStatus::URL_DOWNLOAD && m_urlFileDownloader)
    {
        if (firmwareDownloadStatusExistsForDevice(deviceKey))
        {
            auto firmwareStatus = getFirmwareDownloadStatusForDevice(deviceKey);
            m_urlFileDownloader->abort(firmwareStatus.uri);
            sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ABORTED}, firmwareStatus.channel);

            installAborted(deviceKey);
        }

        return;
    }

    if (status.status == DeviceUpdateStruct::DeviceUpdateStatus::READY ||
        status.status == DeviceUpdateStruct::DeviceUpdateStatus::TRANSFER)
    {
        if (deviceKey == m_gatewayKey)
        {
            sendResponse(FirmwareUpdateResponse{FirmwareUpdateResponse::Status::ABORTED}, m_gatewayKey);

            installAborted(deviceKey);
        }
        else
        {
            sendCommand(FirmwareUpdateCommand(FirmwareUpdateCommand::Type::ABORT), deviceKey);
        }
    }
}

void FirmwareUpdateService::sendResponse(const FirmwareUpdateResponse& response, const std::string& deviceKey)
{
    std::shared_ptr<Message> message = m_protocol.makeMessage(m_gatewayKey, deviceKey, response);

    if (!message)
    {
        LOG(WARN) << "Failed to create firmware update response";
        return;
    }

    m_outboundPlatformMessageHandler.addMessage(message);
}

void FirmwareUpdateService::sendCommand(const FirmwareUpdateCommand& command, const std::string& deviceKey)
{
    std::shared_ptr<Message> message = m_protocol.makeMessage(deviceKey, command);

    if (!message)
    {
        LOG(WARN) << "Failed to create firmware update command";
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(message);
}

void FirmwareUpdateService::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer.pushCommand(std::make_shared<std::function<void()>>(command));
}

void FirmwareUpdateService::addDeviceUpdateStatus(const std::string& deviceKey,
                                                  DeviceUpdateStruct::DeviceUpdateStatus status, bool autoInstall)
{
    m_deviceUpdateStatuses[deviceKey] = {autoInstall, status};
}

void FirmwareUpdateService::setDeviceUpdateStatus(const std::string& deviceKey,
                                                  DeviceUpdateStruct::DeviceUpdateStatus status)
{
    m_deviceUpdateStatuses[deviceKey].status = status;
}

bool FirmwareUpdateService::deviceUpdateStatusExists(const std::string& deviceKey)
{
    return m_deviceUpdateStatuses.find(deviceKey) != m_deviceUpdateStatuses.end();
}

FirmwareUpdateService::DeviceUpdateStruct FirmwareUpdateService::getDeviceUpdateStatus(const std::string& deviceKey)
{
    if (deviceUpdateStatusExists(deviceKey))
    {
        return m_deviceUpdateStatuses.at(deviceKey);
    }

    return FirmwareUpdateService::DeviceUpdateStruct{false, DeviceUpdateStruct::DeviceUpdateStatus::UNKNOWN};
}

void FirmwareUpdateService::removeDeviceUpdateStatus(const std::string& deviceKey)
{
    if (deviceUpdateStatusExists(deviceKey))
    {
        m_deviceUpdateStatuses.erase(m_deviceUpdateStatuses.find(deviceKey));
    }
}

void FirmwareUpdateService::addFirmwareDownloadStatus(const std::string& key, const std::string& channel,
                                                      const std::string& uri,
                                                      FirmwareDownloadStruct::FirmwareDownloadStatus status,
                                                      const std::vector<std::string>& deviceKeys,
                                                      const std::string& firmwareFile)
{
    m_firmwareStatuses[key] = {status, channel, uri, deviceKeys, firmwareFile};
}

FirmwareUpdateService::FirmwareDownloadStruct FirmwareUpdateService::getFirmwareDownloadStatus(const std::string& key)
{
    if (firmwareDownloadStatusExists(key))
    {
        return m_firmwareStatuses.at(key);
    }

    return FirmwareUpdateService::FirmwareDownloadStruct{
      FirmwareDownloadStruct::FirmwareDownloadStatus::UNKNOWN, "", "", {}, ""};
}

void FirmwareUpdateService::setFirmwareDownloadCompletedStatus(const std::string& key, const std::string& firmwareFile)
{
    m_firmwareStatuses[key].downloadedFirmwarePath = firmwareFile;
    m_firmwareStatuses[key].status = FirmwareDownloadStruct::FirmwareDownloadStatus::COMPLETED;
}

bool FirmwareUpdateService::firmwareDownloadStatusExists(const std::string& key)
{
    return m_firmwareStatuses.find(key) != m_firmwareStatuses.end();
}

bool FirmwareUpdateService::firmwareDownloadStatusExistsForDevice(const std::string& deviceKey)
{
    for (auto& pair : m_firmwareStatuses)
    {
        auto& deviceList = pair.second.devices;

        auto it = std::find(deviceList.begin(), deviceList.end(), deviceKey);
        if (it != deviceList.end())
        {
            return true;
        }
    }

    return false;
}

FirmwareUpdateService::FirmwareDownloadStruct FirmwareUpdateService::getFirmwareDownloadStatusForDevice(
  const std::string& deviceKey)
{
    for (auto& pair : m_firmwareStatuses)
    {
        auto& deviceList = pair.second.devices;

        auto it = std::find(deviceList.begin(), deviceList.end(), deviceKey);
        if (it != deviceList.end())
        {
            return pair.second;
        }
    }

    return FirmwareUpdateService::FirmwareDownloadStruct{
      FirmwareDownloadStruct::FirmwareDownloadStatus::UNKNOWN, "", "", {}, ""};
}

void FirmwareUpdateService::removeDeviceFromFirmwareStatus(const std::string& deviceKey)
{
    for (auto& pair : m_firmwareStatuses)
    {
        auto& deviceList = pair.second.devices;

        auto it = std::find(deviceList.begin(), deviceList.end(), deviceKey);
        if (it != deviceList.end())
        {
            deviceList.erase(it);
        }
    }
}

void FirmwareUpdateService::removeFirmwareStatus(const std::string& key)
{
    auto it = m_firmwareStatuses.find(key);
    if (it != m_firmwareStatuses.end())
    {
        for (const auto& deviceKey : it->second.devices)
        {
            removeDeviceUpdateStatus(deviceKey);
        }
        m_firmwareStatuses.erase(it);
    }
}

void FirmwareUpdateService::clearUsedFirmwareFiles()
{
    for (auto it = m_firmwareStatuses.begin(); it != m_firmwareStatuses.end();)
    {
        if (it->second.devices.empty())
        {
            FileSystemUtils::deleteFile(it->second.downloadedFirmwarePath);
            it = m_firmwareStatuses.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
}    // namespace wolkabout
