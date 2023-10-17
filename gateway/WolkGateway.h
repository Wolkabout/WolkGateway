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
#include "core/utility/StringUtils.h"
#include "gateway/WolkGatewayBuilder.h"
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
class OutboundMessageHandler;
class OutboundRetryMessageHandler;

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
class ExternalDataService;
class ExistingDevicesRepository;
class GatewayMessageRouter;
class InMemoryDeviceRepository;
class InternalDataService;
class GatewayPlatformStatusService;
class DevicesService;

class WolkGateway : public connect::WolkSingle
{
    friend class WolkGatewayBuilder;

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
    static WolkGatewayBuilder newBuilder(Device device);

    /**
     * Overridden method from `connect::WolkSingle`. Will attempt to connect to the platform and to the local broker -
     * if the local connectivity has been set up.
     */
    void connect() override;

    /**
     * Overridden method from `connect::WolkSingle`. Will close both the platform and the local connectivity.
     */
    void disconnect() override;

    /**
     * Default getter for the platform connection status.
     *
     * @return The status of the connection with platform.
     */
    bool isPlatformConnected();

    /**
     * Default getter for the local connection status.
     *
     * @return The status of the connection with the local broker. If no connection with a local broker exists, will
     * always be false.
     */
    bool isLocalConnected();

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
    connect::WolkInterfaceType getType() const override;

protected:
    explicit WolkGateway(Device device);

    static std::uint64_t currentRtc();

    void platformDisconnected();

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

    std::atomic<bool> m_localConnected;

    std::shared_ptr<InMemoryDeviceRepository> m_cacheDeviceRepository;
    std::shared_ptr<DeviceRepository> m_persistentDeviceRepository;
    std::shared_ptr<ExistingDevicesRepository> m_existingDevicesRepository;

    // Local connectivity stack
    std::shared_ptr<ConnectivityService> m_localConnectivityService;
    std::shared_ptr<InboundMessageHandler> m_localInboundMessageHandler;
    std::shared_ptr<OutboundMessageHandler> m_localOutboundMessageHandler;

    // Additional connectivity
    std::shared_ptr<MessagePersistence> m_messagePersistence;
    OutboundMessageHandler* m_outboundMessageHandler;
    std::unique_ptr<OutboundRetryMessageHandler> m_outboundRetryMessageHandler;

    // Gateway connectivity manager
    std::shared_ptr<GatewayMessageRouter> m_gatewayMessageRouter;

    // Gateway protocols
    std::unique_ptr<GatewaySubdeviceProtocol> m_platformSubdeviceProtocol;
    std::unique_ptr<GatewaySubdeviceProtocol> m_localSubdeviceProtocol;
    std::unique_ptr<RegistrationProtocol> m_platformRegistrationProtocol;
    std::shared_ptr<GatewayRegistrationProtocol> m_localRegistrationProtocol;
    std::unique_ptr<GatewayPlatformStatusProtocol> m_gatewayPlatformStatusProtocol;

    // Gateway services
    std::shared_ptr<ExternalDataService> m_externalDataService;
    std::shared_ptr<InternalDataService> m_internalDataService;
    std::shared_ptr<GatewayPlatformStatusService> m_gatewayPlatformStatusService;
    std::shared_ptr<DevicesService> m_subdeviceManagementService;
};
}    // namespace gateway
}    // namespace wolkabout

#endif
