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
#ifndef FIRMWAREUPDATESERVICE_H
#define FIRMWAREUPDATESERVICE_H

#include "GatewayInboundDeviceMessageHandler.h"
#include "GatewayInboundPlatformMessageHandler.h"
#include "service/WolkaboutFileDownloader.h"

#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace wolkabout
{
class FirmwareUpdateCommand;
class FirmwareUpdateResponse;
class GatewayFirmwareUpdateProtocol;
class OutboundMessageHandler;

class FirmwareUpdateService : public DeviceMessageListener, public PlatformMessageListener
{
public:
    FirmwareUpdateService(std::string gatewayKey, GatewayFirmwareUpdateProtocol& protocol,
                          OutboundMessageHandler& outboundPlatformMessageHandler,
                          OutboundMessageHandler& outboundDeviceMessageHandler,
                          WolkaboutFileDownloader& wolkaboutFileDownloader);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    const GatewayProtocol& getProtocol() const override;

private:
    void handleFirmwareUpdateCommand(const FirmwareUpdateCommand& command, const std::string deviceKey);

    void handleFirmwareUpdateResponse(const FirmwareUpdateResponse& response, const std::string deviceKey);

    void fileUpload(const std::string& deviceKey, const std::string& name, std::uint_fast64_t size,
                    const std::string& hash, bool autoInstall, const std::string& subChannel);

    void downloadCompleted(const std::string& filePath, const std::string& hash, const std::string& subChannel);

    void downloadFailed(WolkaboutFileDownloader::ErrorCode errorCode, const std::string& hash,
                        const std::string& subChannel);

    void transferFile(const std::string& deviceKey, const std::string& filePath);

    void install(const std::string& deviceKey);

    void installCompleted(const std::string& deviceKey);

    void installAborted(const std::string& deviceKey);

    void installFailed(const std::string& deviceKey);

    void abort(const std::string& deviceKey);

    void sendResponse(const FirmwareUpdateResponse& response, const std::string& deviceKey);

    void sendCommand(const FirmwareUpdateCommand& command, const std::string& deviceKey);

    void addToCommandBuffer(std::function<void()> command);

    const std::string m_gatewayKey;
    GatewayFirmwareUpdateProtocol& m_protocol;

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    OutboundMessageHandler& m_outboundDeviceMessageHandler;

    WolkaboutFileDownloader& m_wolkaboutFileDownloader;

    const std::string m_firmwareDownloadDirectory;

    enum class FirmwareDownloadStatus
    {
        IN_PROGRESS,
        COMPLETED,
        UNKNOWN
    };

    struct FirmwareDownloadStruct
    {
        FirmwareDownloadStatus status;
        std::vector<std::string> devices;
    };

    std::map<std::string, FirmwareDownloadStruct> m_firmwareStatuses;

    struct DeviceUpdateStruct
    {
        bool autoinstall;

        enum class DeviceUpdateStatus
        {
            DOWNLOAD,
            TRANSFER,
            READY,
            INSTALL,
            ERROR,
            UNKNOWN
        } status;
    };

    void addDeviceUpdateStatus(const std::string& deviceKey, DeviceUpdateStruct::DeviceUpdateStatus status,
                               bool autoInstall);
    void setDeviceUpdateStatus(const std::string& deviceKey, DeviceUpdateStruct::DeviceUpdateStatus status);
    bool deviceUpdateStatusExists(const std::string& deviceKey);
    DeviceUpdateStruct getDeviceUpdateStatus(const std::string& deviceKey);
    void removeDeviceUpdateStatus(const std::string& deviceKey);

    // TODO move to database
    std::map<std::string, DeviceUpdateStruct> m_deviceUpdateStatuses;

    CommandBuffer m_commandBuffer;
};
}    // namespace wolkabout

#endif    // FIRMWAREUPDATESERVICE_H
