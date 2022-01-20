/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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

#ifndef WOLK_H
#define WOLK_H

#include "core/model/Device.h"
#include "core/utilities/StringUtils.h"
#include "gateway/WolkBuilder.h"
#include "wolk/WolkSingle.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace wolkabout
{
// List the service
class InboundPlatformMessageHandler;

// List of all the protocols
class GatewayPlatformStatusProtocol;
class GatewayRegistrationProtocol;
class GatewaySubdeviceProtocol;

namespace connect
{
// List of all device services
class DataService;
class ErrorService;
class FileManagementService;
class FirmwareUpdateService;
class PlatformStatusService;
class RegistrationService;
}    // namespace connect

namespace gateway
{
class DeviceRepository;
class ExistingDevicesRepository;
class GatewayMessageRouter;
class ExternalDataService;
// TODO Uncomment
// class InternalDataService;
class GatewayPlatformStatusService;
// TODO Uncomment
// class GatewayRegistrationService;

class WolkGateway : public connect::WolkSingle
{
    friend class WolkBuilder;

public:
    /**
     * Overridden virtual destructor.
     */
    ~WolkGateway() override;

    /**
     * @brief Initiates wolkabout::WolkBuilder that configures device to connectService to WolkAbout IoT Cloud
     * @param device wolkabout::Device
     * @return wolkabout::WolkBuilder instance
     */
    static WolkBuilder newBuilder(Device device);

    /**
     * This is the default getter method for obtaining the platform connection status.
     *
     * @return Platform connection status.
     */
    virtual bool isConnectedToPlatform();

    /**
     * This is the default setter method for setting a callback function that listens to the platform connection status.
     *
     * @param platformConnectionStatusListener The callback function.
     */
    virtual void setPlatformConnectionStatusListener(const std::function<void(bool)>& platformConnectionStatusListener);

    /**
     * @brief connectService Establishes connection with WolkAbout IoT platform
     */
    void connect() override;

    /**
     * @brief disconnect Disconnects from WolkAbout IoT platform
     */
    void disconnect() override;

    /**
     * @brief Publishes data
     */
    void publish() override;

    /**
     * This is the overridden method from the `WolkInterface` interface.
     * This is meant to indicate what type of a `WolkInterface` this object.
     *
     * @return The value `connectService::WolkInterfaceType::Gateway`.
     */
    connect::WolkInterfaceType getType() override;

protected:
    explicit WolkGateway(Device device);

    void addToCommandBuffer(std::function<void()> command);

    static std::uint64_t currentRtc();

    void flushFeeds();

    // callbacks
    void handleFeedUpdate(const std::map<std::uint64_t, std::vector<Reading>>& readings);
    void handleParameterUpdate(const std::vector<Parameter>& parameters);

    void platformDisconnected();

    void publishEverything();
    void publishFirmwareStatus();
    void publishFileList();

    void notifyPlatformConnected();
    void notifyPlatformDisconnected();

    /**
     * Internal method used to connect the platform connectivity service to the platform.
     *
     * @param firstTime Whether this is the first attempt at connecting to the platform.
     */
    void connectPlatform(bool firstTime = false);

    /**
     * Internal method used to connect the local connectivity service to the local broker.
     *
     * @param firstTime Whether this is the first attempt at connecting to the local broker.
     */
    void connectLocal(bool firstTime = false);

    const std::string TAG = "[WolkGateway] -> ";

    std::atomic<bool> m_connected;
    std::function<void(bool)> m_platformConnectionStatusListener;

    std::unique_ptr<DeviceRepository> m_deviceRepository;
    std::unique_ptr<ExistingDevicesRepository> m_existingDevicesRepository;

    // Gateway connectivity manager
    std::shared_ptr<GatewayMessageRouter> m_gatewayMessageRouter;

    // Local connectivity stack
    std::unique_ptr<ConnectivityService> m_localConnectivityService;
    std::unique_ptr<InboundPlatformMessageHandler> m_localConnectivityManager;

    // Gateway protocols
    std::unique_ptr<GatewaySubdeviceProtocol> m_gatewaySubdeviceProtocol;
    std::unique_ptr<RegistrationProtocol> m_registrationProtocol;
    std::unique_ptr<GatewayRegistrationProtocol> m_gatewayRegistrationProtocol;
    std::unique_ptr<GatewayPlatformStatusProtocol> m_gatewayPlatformStatusProtocol;

    // Gateway services
    std::shared_ptr<ExternalDataService> m_externalDataService;
    // TODO Uncomment
    //    std::shared_ptr<InternalDataService> m_internalDataService;
    std::shared_ptr<GatewayPlatformStatusService> m_gatewayPlatformStatusService;
    // TODO Uncomment
    //    std::shared_ptr<GatewayRegistrationService> m_gatewayRegistrationService;

    // External API entities
    std::shared_ptr<DataProvider> m_dataProvider;

    // Internal utilities
    std::mutex m_lock;
    std::unique_ptr<CommandBuffer> m_commandBuffer;
};
}    // namespace gateway
}    // namespace wolkabout

#endif
