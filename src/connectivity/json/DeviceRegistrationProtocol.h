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

#ifndef DEVICEREGISTRATIONPROTOCOL_H
#define DEVICEREGISTRATIONPROTOCOL_H

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class DeviceRegistrationRequestDto;
class DeviceRegistrationResponseDto;
class DeviceReregistrationResponseDto;

class DeviceRegistrationProtocol
{
public:
    ~DeviceRegistrationProtocol() = delete;

    static const std::string& getName();

    static const std::vector<std::string>& getDeviceTopics();
    static const std::vector<std::string>& getPlatformTopics();

    static std::shared_ptr<Message> makeMessage(const std::string& gatewayKey, const std::string& deviceKey,
                                                const DeviceRegistrationRequestDto& request);
    static std::shared_ptr<Message> makeMessage(const std::string& gatewayKey, const std::string& deviceKey,
                                                const DeviceRegistrationResponseDto& request);
    static std::shared_ptr<Message> makeMessage(const std::string& gatewayKey,
                                                const DeviceReregistrationResponseDto& response);

    static std::shared_ptr<DeviceRegistrationRequestDto> makeRegistrationRequest(std::shared_ptr<Message> message);
    static std::shared_ptr<DeviceRegistrationResponseDto> makeRegistrationResponse(std::shared_ptr<Message> message);

    static bool isMessageToPlatform(const std::string& channel);

    static bool isMessageFromPlatform(const std::string& channel);

    static bool isRegistrationRequest(std::shared_ptr<Message> message);
    static bool isRegistrationResponse(std::shared_ptr<Message> message);

    static bool isReregistrationRequest(std::shared_ptr<Message> message);
    static bool isReregistrationResponse(std::shared_ptr<Message> message);

    static std::string extractDeviceKeyFromChannel(const std::string& channel);

private:
    static const std::string NAME;

    static const std::string CHANNEL_DELIMITER;
    static const std::string CHANNEL_WILDCARD;
    static const std::string GATEWAY_PATH_PREFIX;
    static const std::string DEVICE_PATH_PREFIX;
    static const std::string DEVICE_TO_PLATFORM_DIRECTION;
    static const std::string PLATFORM_TO_DEVICE_DIRECTION;

    static const std::string DEVICE_REGISTRATION_REQUEST_TOPIC_ROOT;
    static const std::string DEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT;
    static const std::string DEVICE_REREGISTRATION_RESPONSE_TOPIC_ROOT;
    static const std::string DEVICE_REREGISTRATION_REQUEST_TOPIC_ROOT;

    static const std::vector<std::string> DEVICE_TOPICS;
    static const std::vector<std::string> PLATFORM_TOPICS;

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
