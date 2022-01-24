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

#ifndef SUBDEVICEREGISTRATIONSERVICE_H
#define SUBDEVICEREGISTRATIONSERVICE_H

#include "core/MessageListener.h"
#include "gateway/GatewayMessageListener.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace wolkabout
{
class ConnectivityService;
class GatewayRegistrationProtocol;
class OutboundMessageHandler;
class OutboundRetryMessageHandler;
class RegisteredDevicesResponseMessage;
class RegistrationProtocol;

namespace gateway
{
class DeviceRepository;

/**
 * This struct is used to cache the requests that are sent out, and connect them with the responses.
 * The response can either be a condition variable waiting, or a simple callback.
 */
struct RegisteredDevicesRequestParameters
{
public:
    RegisteredDevicesRequestParameters(const std::chrono::milliseconds& timestampFrom, std::string deviceType,
                                       std::string externalId);

    const std::chrono::milliseconds& getTimestampFrom() const;

    const std::string& getDeviceType() const;

    const std::string& getExternalId() const;

    bool operator==(const RegisteredDevicesRequestParameters& value) const;

private:
    std::chrono::milliseconds m_timestampFrom;
    std::string m_deviceType;
    std::string m_externalId;
};

/**
 * This struct is the hash implementation for the request parameters used by the unordered_map.
 */
struct RegisteredDevicesRequestParametersHash
{
    std::uint64_t operator()(const RegisteredDevicesRequestParameters& params) const;
};

/**
 * This service is used to manage anything regarding sub-devices. This service is used to route the requests from
 * sub-devices to the platform regarding registration, deletion, and also registered devices. This service will also
 * keep a cache of registered devices.
 */
class SubdeviceManagementService : public GatewayMessageListener, public MessageListener
{
public:
    /**
     * Default parameter constructor.
     *
     * @param gatewayKey The key of the gateway device this service will belong to.
     * @param platformRegistrationProtocol The platform registration protocol used for exchanging messages with the
     * platform.
     * @param localRegistrationProtocol The local registration protocol used for exchanging messages with sub-devices.
     * @param outboundPlatformMessageHandler The communication service for platform communication.
     * @param outboundDeviceMessageHandler The communication service for local communication.
     * @param deviceRepository The repository for storing device information.
     */
    SubdeviceManagementService(std::string gatewayKey, RegistrationProtocol& platformRegistrationProtocol,
                               GatewayRegistrationProtocol& localRegistrationProtocol,
                               OutboundRetryMessageHandler& outboundPlatformMessageHandler,
                               OutboundMessageHandler& outboundDeviceMessageHandler,
                               DeviceRepository& deviceRepository);

    /**
     * Overridden destructor.
     */
    ~SubdeviceManagementService() override;

    /**
     * Internal method that is used to send out the request to obtain the list of requested devices.
     *
     * @param timestampFrom The timestamp from which further we want registered devices to appear in the list.
     * @param deviceType The only acceptable device types.
     * @param externalId The only acceptable external id.
     */
    bool sendOutRegisteredDevicesRequest(std::chrono::milliseconds timestampFrom, const std::string& deviceType = {},
                                         const std::string& externalId = {});

    /**
     * This method is overridden from the `wolkabout::MessageListener` interface.
     * This is the method that is invoked once a message has been received via an `InboundMessageHandler`.
     * This is used for receiving messages from the local connectivity service.
     *
     * @param message The message that has been sent out by a subdevice.
     */
    void messageReceived(std::shared_ptr<Message> message) override;

    /**
     * This method is overridden from the `wolkabout::MessageListener` interface.
     * This is the method that the `InboundMessageHandler` will invoke to get information about the protocol we are
     * following.
     *
     * @return Reference to the protocol we are using.
     */
    const Protocol& getProtocol() override;

    /**
     * This method is overridden from the `gateway::GatewayMessageListener` interface.
     * This is the method that is invoked once the gateway receives a message via the `GatewayMessageRouter`.
     * This is used for receiving messages from the platform connectivity service.
     *
     * @param messages The messages that the platform has sent out for us.
     */
    void receiveMessages(const std::vector<GatewaySubdeviceMessage>& messages) override;

    /**
     * This method is overridden from the `gateway::GatewayMessageListener` interface.
     * This is the method via which we tell MessageTypes that we are interested in listening.
     *
     * @return The list of MessageTypes we can handle.
     */
    std::vector<MessageType> getMessageTypes() override;

private:
    // Device information and protocols
    const std::string m_gatewayKey;
    RegistrationProtocol& m_platformProtocol;
    GatewayRegistrationProtocol& m_localProtocol;

    // Outgoing communication entities
    OutboundRetryMessageHandler& m_outboundPlatformRetryMessageHandler;
    OutboundMessageHandler& m_outboundLocalMessageHandler;

    // Device information storage
    DeviceRepository& m_deviceRepository;

    // Storage for request objects
    std::unordered_map<RegisteredDevicesRequestParameters,
                       std::function<void(std::unique_ptr<RegisteredDevicesResponseMessage>)>,
                       RegisteredDevicesRequestParametersHash>
      m_requests;
};
}    // namespace gateway
}    // namespace wolkabout

#endif
