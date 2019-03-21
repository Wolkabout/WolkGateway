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
#ifndef FIRMWAREUPDATESERVICE_H
#define FIRMWAREUPDATESERVICE_H

#include "GatewayInboundDeviceMessageHandler.h"
#include "GatewayInboundPlatformMessageHandler.h"
#include "model/FirmwareUpdateStatus.h"
#include "utilities/CommandBuffer.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class FileRepository;
class FirmwareInstaller;
class FirmwareUpdateAbort;
class FirmwareUpdateInstall;
class FirmwareUpdateStatus;
class FirmwareVersion;
class GatewayFirmwareUpdateProtocol;
class JsonDFUProtocol;
class OutboundMessageHandler;

class FirmwareUpdateService : public DeviceMessageListener, public PlatformMessageListener
{
public:
    FirmwareUpdateService(std::string gatewayKey, JsonDFUProtocol& protocol,
                          GatewayFirmwareUpdateProtocol& gatewayProtocol, FileRepository& fileRepository,
                          OutboundMessageHandler& outboundPlatformMessageHandler,
                          OutboundMessageHandler& outboundDeviceMessageHandler);

    FirmwareUpdateService(std::string gatewayKey, JsonDFUProtocol& protocol,
                          GatewayFirmwareUpdateProtocol& gatewayProtocol, FileRepository& fileRepository,
                          OutboundMessageHandler& outboundPlatformMessageHandler,
                          OutboundMessageHandler& outboundDeviceMessageHandler,
                          std::shared_ptr<FirmwareInstaller> firmwareInstaller, std::string currentFirmwareVersion);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    const Protocol& getProtocol() const override;

    const GatewayProtocol& getGatewayProtocol() const override;

    void reportFirmwareUpdateResult();

    void publishFirmwareVersion();

private:
    void handleFirmwareUpdateCommand(const FirmwareUpdateInstall& command);
    void handleFirmwareUpdateCommand(const FirmwareUpdateAbort& command);

    void handleFirmwareUpdateStatus(const FirmwareUpdateStatus& status);
    void handleFirmwareVersion(const FirmwareVersion& version);

    void install(const std::vector<std::string>& deviceKeys, const std::string& fileName);
    void installGatewayFirmware(const std::string& filePath);
    void installDeviceFirmware(const std::string& deviceKey, const std::string& filePath);

    void installationInProgress(const std::vector<std::string>& deviceKeys);
    void installationCompleted(const std::vector<std::string>& deviceKeys);
    void installationAborted(const std::vector<std::string>& deviceKeys);
    void installationFailed(const std::vector<std::string>& deviceKeys,
                            WolkOptional<FirmwareUpdateStatus::Error> errorCode);

    void abort(const std::vector<std::string>& deviceKeys);
    void abortGatewayFirmware();
    void abortDeviceFirmware(const std::string& deviceKey);

    void sendStatus(const FirmwareUpdateStatus& status);
    void sendVersion(const FirmwareVersion& version);

    void sendCommand(const FirmwareUpdateInstall& command);
    void sendCommand(const FirmwareUpdateAbort& command);

    void addToCommandBuffer(std::function<void()> command);

    const std::string m_gatewayKey;
    JsonDFUProtocol& m_protocol;
    GatewayFirmwareUpdateProtocol& m_gatewayProtocol;

    FileRepository& m_fileRepository;

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    OutboundMessageHandler& m_outboundDeviceMessageHandler;

    std::shared_ptr<FirmwareInstaller> m_firmwareInstaller;
    const std::string m_currentFirmwareVersion;

    //    std::shared_ptr<UrlFileDownloader> m_urlFileDownloader;
    //
    //    struct FirmwareDownloadStruct
    //    {
    //        enum class FirmwareDownloadStatus
    //        {
    //            IN_PROGRESS,
    //            COMPLETED,
    //            UNKNOWN
    //        } status;
    //
    //        std::string channel;
    //        std::string uri;
    //        std::vector<std::string> devices;
    //        std::string downloadedFirmwarePath;
    //    };
    //
    //    std::map<std::string, FirmwareDownloadStruct> m_firmwareStatuses;
    //
    //    struct DeviceUpdateStruct
    //    {
    //        bool autoinstall;
    //
    //        enum class DeviceUpdateStatus
    //        {
    //            WOLK_DOWNLOAD,
    //            URL_DOWNLOAD,
    //            TRANSFER,
    //            READY,
    //            INSTALL,
    //            ERROR,
    //            UNKNOWN
    //        } status;
    //    };
    //
    //    void addDeviceUpdateStatus(const std::string& deviceKey, DeviceUpdateStruct::DeviceUpdateStatus status,
    //                               bool autoInstall);
    //    void setDeviceUpdateStatus(const std::string& deviceKey, DeviceUpdateStruct::DeviceUpdateStatus status);
    //    bool deviceUpdateStatusExists(const std::string& deviceKey);
    //    DeviceUpdateStruct getDeviceUpdateStatus(const std::string& deviceKey);
    //    void removeDeviceUpdateStatus(const std::string& deviceKey);
    //
    //    void addFirmwareDownloadStatus(const std::string& key, const std::string& channel, const std::string& uri,
    //                                   FirmwareDownloadStruct::FirmwareDownloadStatus status,
    //                                   const std::vector<std::string>& deviceKeys,
    //                                   const std::string& downloadedFirmwarePath = "");
    //    void setFirmwareDownloadCompletedStatus(const std::string& key, const std::string& firmwareFile);
    //    bool firmwareDownloadStatusExists(const std::string& key);
    //    bool firmwareDownloadStatusExistsForDevice(const std::string& deviceKey);
    //    FirmwareDownloadStruct getFirmwareDownloadStatus(const std::string& key);
    //    FirmwareDownloadStruct getFirmwareDownloadStatusForDevice(const std::string& deviceKey);
    //    void removeDeviceFromFirmwareStatus(const std::string& deviceKey);
    //    void removeFirmwareStatus(const std::string& key);
    //    void clearUsedFirmwareFiles();
    //
    //    // TODO move to database
    //    std::map<std::string, DeviceUpdateStruct> m_deviceUpdateStatuses;

    CommandBuffer m_commandBuffer;

    static const constexpr char* FIRMWARE_VERSION_FILE = ".dfu-version";
};
}    // namespace wolkabout

#endif    // FIRMWAREUPDATESERVICE_H
