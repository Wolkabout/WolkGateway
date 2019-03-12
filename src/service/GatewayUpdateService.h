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

#ifndef GATEWAYUPDATESERVICE_H
#define GATEWAYUPDATESERVICE_H

#include "InboundPlatformMessageHandler.h"
#include "OutboundRetryMessageHandler.h"

#include <functional>
#include <memory>
#include <mutex>
#include <string>

namespace wolkabout
{
class DetailedDevice;
class DeviceRepository;
class GatewaySubdeviceRegistrationProtocol;
class GatewayUpdateResponse;
class OutboundMessageHandler;

class GatewayUpdateService : public PlatformMessageListener
{
public:
    GatewayUpdateService(std::string gatewayKey, GatewaySubdeviceRegistrationProtocol& protocol,
                         DeviceRepository& deviceRepository, OutboundMessageHandler& outboundPlatformMessageHandler);
    ~GatewayUpdateService();

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    const GatewayProtocol& getProtocol() const override;

    void onGatewayUpdated(std::function<void()> callback);

    void updateGateway(const DetailedDevice& device);

private:
    void handleUpdateResponse(const GatewayUpdateResponse& response);

    const std::string m_gatewayKey;
    GatewaySubdeviceRegistrationProtocol& m_protocol;

    DeviceRepository& m_deviceRepository;

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    OutboundRetryMessageHandler m_platformRetryMessageHandler;

    std::function<void()> m_onGatewayUpdated;

    std::mutex m_mutex;
    std::unique_ptr<DetailedDevice> m_pendingUpdateRequest;
};
}    // namespace wolkabout

#endif    // GATEWAYUPDATESERVICE_H
