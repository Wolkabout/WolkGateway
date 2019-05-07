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

#ifndef SUBDEVICEREGISTRATIONSERVICE_H
#define SUBDEVICEREGISTRATIONSERVICE_H

#include "GatewayInboundDeviceMessageHandler.h"
#include "GatewayInboundPlatformMessageHandler.h"
#include "OutboundRetryMessageHandler.h"
#include "model/SubdeviceRegistrationRequest.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace wolkabout
{
class DetailedDevice;
class DeviceRepository;
class GatewaySubdeviceRegistrationProtocol;
class Message;
class OutboundMessageHandler;
class RegistrationProtocol;
class SubdeviceRegistrationResponse;

class SubdeviceRegistrationService : public DeviceMessageListener, public PlatformMessageListener
{
public:
    SubdeviceRegistrationService(std::string gatewayKey, RegistrationProtocol& protocol,
                                 GatewaySubdeviceRegistrationProtocol& gatewayProtocol,
                                 DeviceRepository& deviceRepository,
                                 OutboundMessageHandler& outboundPlatformMessageHandler,
                                 OutboundMessageHandler& outboundDeviceMessageHandler);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    void deviceMessageReceived(std::shared_ptr<Message> message) override;

    const GatewayProtocol& getGatewayProtocol() const override;

    const Protocol& getProtocol() const override;

    void onDeviceRegistered(std::function<void(const std::string& deviceKey)> onDeviceRegistered);

    virtual void deleteDevicesOtherThan(const std::vector<std::string>& devicesKeys);

    virtual void registerPostponedDevices();

protected:
    void invokeOnDeviceRegisteredListener(const std::string& deviceKey) const;

private:
    void handleSubdeviceRegistrationRequest(const std::string& deviceKey, const SubdeviceRegistrationRequest& request);

    void handleSubdeviceRegistrationResponse(const std::string& deviceKey,
                                             const SubdeviceRegistrationResponse& response);

    void addToPostponedSubdeviceRegistrationRequests(const std::string& deviceKey,
                                                     const SubdeviceRegistrationRequest& request);

    const std::string m_gatewayKey;
    RegistrationProtocol& m_protocol;
    GatewaySubdeviceRegistrationProtocol& m_gatewayProtocol;

    DeviceRepository& m_deviceRepository;

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    OutboundMessageHandler& m_outboundDeviceMessageHandler;

    OutboundRetryMessageHandler m_platformRetryMessageHandler;

    std::function<void(const std::string& deviceKey)> m_onDeviceRegistered;

    std::recursive_mutex m_devicesAwaitingRegistrationResponseMutex;
    std::map<std::string, std::unique_ptr<DetailedDevice>> m_devicesAwaitingRegistrationResponse;

    std::mutex m_devicesWithPostponedRegistrationMutex;
    std::map<std::string, std::unique_ptr<SubdeviceRegistrationRequest>> m_devicesWithPostponedRegistration;
};
}    // namespace wolkabout

#endif
