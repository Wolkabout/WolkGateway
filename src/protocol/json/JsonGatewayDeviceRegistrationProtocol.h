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

#ifndef JSONGATEWAYDEVICEREGISTRATIONPROTOCOL_H
#define JSONGATEWAYDEVICEREGISTRATIONPROTOCOL_H

#include "protocol/GatewayDeviceRegistrationProtocol.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class DeviceRegistrationRequest;
class DeviceRegistrationResponse;
class DeviceReregistrationResponse;

class JsonGatewayDeviceRegistrationProtocol : public GatewayDeviceRegistrationProtocol
{
public:
    const std::string& getName() const override;
    std::vector<std::string> getInboundPlatformChannels() const override;
    std::vector<std::string> getInboundPlatformChannelsForGatewayKey(const std::string& gatewayKey) const override;
    std::vector<std::string> getInboundPlatformChannelsForKeys(const std::string& gatewayKey,
                                                               const std::string& deviceKey) const override;
    std::vector<std::string> getInboundDeviceChannels() const override;
    std::vector<std::string> getInboundDeviceChannelsForDeviceKey(const std::string& deviceKey) const override;
    std::string extractDeviceKeyFromChannel(const std::string& topic) const override;
    bool isMessageToPlatform(const Message& channel) const override;
    bool isMessageFromPlatform(const Message& channel) const override;

    std::unique_ptr<Message> makeMessage(const std::string& gatewayKey, const std::string& deviceKey,
                                         const DeviceRegistrationRequest& request) const override;
    std::unique_ptr<Message> makeMessage(const std::string& deviceKey,
                                         const DeviceRegistrationResponse& request) const override;
    std::unique_ptr<Message> makeMessage(const std::string& gatewayKey, const std::string& deviceKey,
                                         const DeviceRegistrationResponse& request) const override;
    std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                         const DeviceReregistrationResponse& response) const override;
    std::unique_ptr<Message> makeDeviceReregistrationRequestForDevice() const override;
    std::unique_ptr<Message> makeDeviceReregistrationRequestForGateway(const std::string& gatewayKey) const override;
    std::unique_ptr<Message> makeDeviceDeletionRequestMessage(const std::string& gatewayKey,
                                                              const std::string& deviceKey) const override;
    std::unique_ptr<DeviceRegistrationRequest> makeRegistrationRequest(const Message& message) const override;
    std::unique_ptr<DeviceRegistrationResponse> makeRegistrationResponse(const Message& message) const override;
    bool isRegistrationRequest(const Message& message) const override;
    bool isRegistrationResponse(const Message& message) const override;
    bool isReregistrationRequest(const Message& message) const override;
    bool isReregistrationResponse(const Message& message) const override;
    bool isDeviceDeletionRequest(const Message& message) const override;
    bool isDeviceDeletionResponse(const Message& message) const override;

    std::string getResponseChannel(const Message& message, const std::string& gatewayKey,
                                   const std::string& deviceKey) const override;

private:
    static const std::string NAME;

    static const std::string CHANNEL_DELIMITER;
    static const std::string CHANNEL_MULTI_LEVEL_WILDCARD;
    static const std::string CHANNEL_SINGLE_LEVEL_WILDCARD;

    static const std::string GATEWAY_PATH_PREFIX;
    static const std::string DEVICE_PATH_PREFIX;
    static const std::string DEVICE_TO_PLATFORM_DIRECTION;
    static const std::string PLATFORM_TO_DEVICE_DIRECTION;

    static const std::string DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT;
    static const std::string DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT;
    static const std::string DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT;
    static const std::string DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT;

    static const std::string DEVICE_DELETION_REQUEST_TOPIC_ROOT;
    static const std::string DEVICE_DELETION_RESPONSE_TOPIC_ROOT;

    static const std::string REGISTRATION_RESPONSE_OK;
    static const std::string REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT;
    static const std::string REGISTRATION_RESPONSE_ERROR_MANIFEST_CONFLICT;
    static const std::string REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED;
    static const std::string REGISTRATION_RESPONSE_ERROR_READING_PAYLOAD;
    static const std::string REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND;
    static const std::string REGISTRATION_RESPONSE_ERROR_NO_GATEWAY_MANIFEST;
};
}    // namespace wolkabout

#endif
