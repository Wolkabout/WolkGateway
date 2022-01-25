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
#include <condition_variable>

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
class ExistingDevicesRepository;

/**
 * This struct is used to cache the requests that are sent out, and connect them with the responses.
 * The response can either be a condition variable waiting, or a simple callback.
 */
struct RegisteredDevicesRequestParameters
{
public:
    explicit RegisteredDevicesRequestParameters(const std::chrono::milliseconds& timestampFrom,
                                                std::string deviceType = {}, std::string externalId = {});

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
 * This object contains the definition of what should be done when a specific RegisteredDeviceRequest receives a
 * response.
 */
struct RegisteredDevicesRequestCallback
{
    RegisteredDevicesRequestCallback() = default;

    explicit RegisteredDevicesRequestCallback(
      std::function<void(std::unique_ptr<RegisteredDevicesResponseMessage>)> lambda);

    explicit RegisteredDevicesRequestCallback(std::weak_ptr<std::condition_variable> conditionVariable);

    const std::chrono::milliseconds& getSentTime() const;

    const std::function<void(std::unique_ptr<RegisteredDevicesResponseMessage>)>& getLambda() const;

    const std::weak_ptr<std::condition_variable>& getConditionVariable() const;

private:
    // Timestamp when the request was sent
    std::chrono::milliseconds m_sentTime =
      std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());

    // Potential lambda expression that needs to be invoked
    std::function<void(std::unique_ptr<RegisteredDevicesResponseMessage>)> m_lambda;

    // Potential condition variable that needs to be notified
    std::weak_ptr<std::condition_variable> m_conditionVariable;
};

/**
 * This service is used to manage anything regarding sub-devices. This service is used to route the requests from
 * sub-devices to the platform regarding registration, deletion, and also registered devices. This service will also
 * keep a cache of registered devices.
 */
class DevicesService : public GatewayMessageListener, public MessageListener
{
public:
    /**
     * Default parameter constructor.
     *
     * @param gatewayKey The key of the gateway device this service will belong to.
     * @param platformRegistrationProtocol The platform registration protocol used for exchanging messages with the
     * platform.
     * @param outboundPlatformMessageHandler The communication service for platform communication.
     * @param outboundPlatformRetryMessageHandler The communication service for retrying platform communication.
     * @param localRegistrationProtocol The local registration protocol used for exchanging messages with sub-devices.
     * @param outboundDeviceMessageHandler The communication service for local communication.
     * @param deviceRepository The repository for storing device information.
     */
    DevicesService(std::string gatewayKey, RegistrationProtocol& platformRegistrationProtocol,
                   OutboundMessageHandler& outboundPlatformMessageHandler,
                   OutboundRetryMessageHandler& outboundPlatformRetryMessageHandler,
                   std::shared_ptr<GatewayRegistrationProtocol> localRegistrationProtocol = nullptr,
                   std::shared_ptr<OutboundMessageHandler> outboundDeviceMessageHandler = nullptr,
                   std::shared_ptr<DeviceRepository> deviceRepository = nullptr);

    /**
     * Overridden destructor.
     */
    ~DevicesService() override;

    /**
     * This is the method that should be run when the service is created and can use the connectivity objects.
     * This method will check when the DeviceRepository was last updated, and will request the list of devices that have
     * been registered since the last request.
     *
     * The first time this method is invoked, it will request the entire list of devices registered, from the start of
     * the existence of the platform - to now. This might take a while, and might be a lot of data.
     */
    void updateDeviceCache();

    /**
     * Internal method that is used to send out the request to obtain the list of requested devices.
     *
     * @param parameters The parameter by which the devices will be queried.
     * @param callback The callback object that defines what will be done once a response has been received.
     */
    bool sendOutRegisteredDevicesRequest(RegisteredDevicesRequestParameters parameters,
                                         RegisteredDevicesRequestCallback callback = {});

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
    // Logging tag
    const std::string TAG = "[DevicesService] -> ";

    // Device information and protocols
    const std::string m_gatewayKey;

    // Required platform entities
    RegistrationProtocol& m_platformProtocol;
    OutboundMessageHandler& m_outboundPlatformMessageHandler;
    OutboundRetryMessageHandler& m_outboundPlatformRetryMessageHandler;

    // Optional local connectivity entities
    std::shared_ptr<GatewayRegistrationProtocol> m_localProtocol;
    std::shared_ptr<OutboundMessageHandler> m_outboundLocalMessageHandler;

    // Optional device repository
    std::shared_ptr<DeviceRepository> m_deviceRepository;

    // Storage for request objects
    std::unordered_map<RegisteredDevicesRequestParameters, RegisteredDevicesRequestCallback,
                       RegisteredDevicesRequestParametersHash>
      m_requests;
};
}    // namespace gateway
}    // namespace wolkabout

#endif
