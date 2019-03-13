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

#ifndef JSONGATEWAYSUBDEVICEREGISTRATIONPROTOCOL_H
#define JSONGATEWAYSUBDEVICEREGISTRATIONPROTOCOL_H

#include "protocol/GatewaySubdeviceRegistrationProtocol.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class SubdeviceRegistrationRequest;
class SubdeviceRegistrationResponse;
class DeviceReregistrationResponse;

class JsonGatewaySubdeviceRegistrationProtocol : public GatewaySubdeviceRegistrationProtocol
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

    std::unique_ptr<Message> makeMessage(const std::string& deviceKey,
                                         const SubdeviceRegistrationResponse& request) const override;
    std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                         const SubdeviceRegistrationRequest& request) const override;
    std::unique_ptr<Message> makeMessage(const std::string& gatewayKey,
                                         const GatewayUpdateRequest& request) const override;

    std::unique_ptr<Message> makeSubdeviceDeletionRequestMessage(const std::string& gatewayKey,
                                                                 const std::string& deviceKey) const override;
    std::unique_ptr<GatewayUpdateResponse> makeGatewayUpdateResponse(const Message& message) const override;
    std::unique_ptr<SubdeviceRegistrationRequest> makeSubdeviceRegistrationRequest(
      const Message& message) const override;
    std::unique_ptr<SubdeviceRegistrationResponse> makeSubdeviceRegistrationResponse(
      const Message& message) const override;

    bool isSubdeviceRegistrationRequest(const Message& message) const override;
    bool isSubdeviceRegistrationResponse(const Message& message) const override;
    bool isGatewayUpdateResponse(const Message& message) const override;
    bool isGatewayUpdateRequest(const Message& message) const override;
    bool isSubdeviceDeletionRequest(const Message& message) const override;
    bool isSubdeviceDeletionResponse(const Message& message) const override;

    std::string getResponseChannel(const Message& message, const std::string& gatewayKey,
                                   const std::string& deviceKey) const override;

private:
    static const std::string NAME;

    static const std::string CHANNEL_DELIMITER;
    static const std::string CHANNEL_MULTI_LEVEL_WILDCARD;
    static const std::string CHANNEL_SINGLE_LEVEL_WILDCARD;
    static const std::string CHANNEL_WILDCARD;

    static const std::string GATEWAY_PATH_PREFIX;
    static const std::string DEVICE_PATH_PREFIX;
    static const std::string DEVICE_TO_PLATFORM_DIRECTION;
    static const std::string PLATFORM_TO_DEVICE_DIRECTION;

    static const std::string SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT;

    static const std::string GATEWAY_UPDATE_REQUEST_TOPIC_ROOT;
    static const std::string GATEWAY_UPDATE_RESPONSE_TOPIC_ROOT;

    static const std::string SUBDEVICE_DELETION_REQUEST_TOPIC_ROOT;
    static const std::string SUBDEVICE_DELETION_RESPONSE_TOPIC_ROOT;

    static const std::vector<std::string> INBOUND_CHANNELS;

    static const std::string GATEWAY_UPDATE_RESPONSE_OK;
    static const std::string GATEWAY_UPDATE_RESPONSE_ERROR_GATEWAY_NOT_FOUND;
    static const std::string GATEWAY_UPDATE_RESPONSE_ERROR_NOT_A_GATEWAY;
    static const std::string GATEWAY_UPDATE_RESPONSE_ERROR_VALIDATION_ERROR;
    static const std::string GATEWAY_UPDATE_RESPONSE_ERROR_INVALID_DTO;
    static const std::string GATEWAY_UPDATE_RESPONSE_ERROR_KEY_MISSING;
    static const std::string GATEWAY_UPDATE_RESPONSE_ERROR_UNKNOWN;

    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_OK;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_ERROR_NOT_A_GATEWAY;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_ERROR_VALIDATION_ERROR;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_ERROR_INVALID_DTO;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_ERROR_KEY_MISSING;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_ERROR_SUBDEVICE_MANAGEMENT_FORBIDDEN;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_ERROR_UNKNOWN;
};
}    // namespace wolkabout

#endif
