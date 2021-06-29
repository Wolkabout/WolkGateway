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

#ifndef DATASERVICE_H
#define DATASERVICE_H

#include "ConnectionStatusListener.h"
#include "InboundDeviceMessageHandler.h"
#include "InboundPlatformMessageHandler.h"
#include "OutboundMessageHandler.h"

#include <memory>
#include <mutex>
#include <string>

namespace wolkabout
{
class DataProtocol;
class GatewayDataProtocol;
class MessageListener;

class DataService : public PlatformMessageListener, public OutboundMessageHandler
{
public:
    virtual ~DataService() = default;

    DataService(const std::string& gatewayKey, DataProtocol& protocol, GatewayDataProtocol& gatewayProtocol,
                OutboundMessageHandler& outboundPlatformMessageHandler, MessageListener* gatewayDevice = nullptr);

    void platformMessageReceived(std::shared_ptr<Message> message) override;

    const Protocol& getProtocol() const override;

    void addMessage(std::shared_ptr<Message> message) override;

    void setGatewayMessageListener(MessageListener* gatewayDevice);

    virtual void requestActuatorStatusesForDevice(const std::string& deviceKey) = 0;
    virtual void requestActuatorStatusesForAllDevices() = 0;

protected:
    virtual void routeDeviceToPlatformMessage(std::shared_ptr<Message> message);

    const std::string m_gatewayKey;
    DataProtocol& m_protocol;
    GatewayDataProtocol& m_gatewayProtocol;

private:
    virtual void handleMessageForDevice(std::shared_ptr<Message> message) = 0;
    virtual void handleMessageForGateway(std::shared_ptr<Message> message);

    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    MessageListener* m_gatewayDevice;
    std::mutex m_messageListenerMutex;
};
}    // namespace wolkabout

#endif
