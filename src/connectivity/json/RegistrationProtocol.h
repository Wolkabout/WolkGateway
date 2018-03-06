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

#ifndef REGISTRATIONPROTOCOL_H
#define REGISTRATIONPROTOCOL_H

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class DeviceRegistrationRequestDto;
class DeviceRegistrationResponseDto;
class DeviceReregistrationResponseDto;

class RegistrationProtocol
{
public:
    ~RegistrationProtocol() = delete;

    static const std::string& getName();

    static const std::vector<std::string>& getDeviceTopics();
    static const std::vector<std::string>& getPlatformTopics();

    static std::shared_ptr<Message> make(const std::string& gatewayKey, const std::string& deviceKey,
                                         const DeviceRegistrationRequestDto& request);
    static std::shared_ptr<Message> make(const std::string& gatewayKey, const std::string& deviceKey,
                                         const DeviceReregistrationResponseDto& response);

    static std::shared_ptr<DeviceRegistrationRequestDto> makeRegistrationRequest(std::shared_ptr<Message> message);
    static std::shared_ptr<DeviceRegistrationResponseDto> makeRegistrationResponse(std::shared_ptr<Message> message);

    static bool isGatewayToPlatformMessage(const std::string& topic, const std::string& gatewayKey);
    static bool isDeviceToPlatformMessage(const std::string& topic);
    static bool isMessageToPlatform(const std::string& topic, const std::string& gatewayKey);

    static bool isPlatformToGatewayMessage(const std::string& topic, const std::string& gatewayKey);
    static bool isPlatformToDeviceMessage(const std::string& topic, const std::string& gatewayKey);
    static bool isMessageFromPlatform(const std::string& topic, const std::string& gatewayKey);

    static bool isRegistrationRequest(std::shared_ptr<Message> message);
    static bool isRegistrationResponse(std::shared_ptr<Message> message);

    static bool isReregistrationRequest(std::shared_ptr<Message> message);
    static bool isReregistrationResponse(std::shared_ptr<Message> message);

    static std::string getDeviceKeyFromChannel(const std::string& channel);

private:
    static const std::string m_name;

    static const std::vector<std::string> m_devicTopics;
    static const std::vector<std::string> m_platformTopics;

    static const std::vector<std::string> m_deviceMessageTypes;
    static const std::vector<std::string> m_platformMessageTypes;

    static const std::string REGISTRATION_RESPONSE_OK;
    static const std::string REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT;
    static const std::string REGISTRATION_RESPONSE_ERROR_MANIFEST_CONFLICT;
    static const std::string REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED;
    static const std::string REGISTRATION_RESPONSE_ERROR_READING_PAYLOAD;
    static const std::string REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND;
    static const std::string REGISTRATION_RESPONSE_ERROR_NO_GATEWAY_MANIFEST;

    static constexpr int DIRRECTION_POS = 0;
    static constexpr int TYPE_POS = 1;
    static constexpr int GATEWAY_TYPE_POS = 2;
    static constexpr int GATEWAY_KEY_POS = 3;
    static constexpr int DEVICE_TYPE_POS = 2;
    static constexpr int DEVICE_KEY_POS = 3;
    static constexpr int GATEWAY_DEVICE_TYPE_POS = 4;
    static constexpr int GATEWAY_DEVICE_KEY_POS = 5;
    static constexpr int GATEWAY_REFERENCE_TYPE_POS = 4;
    static constexpr int GATEWAY_REFERENCE_VALUE_POS = 5;
    static constexpr int DEVICE_REFERENCE_TYPE_POS = 4;
    static constexpr int DEVICE_REFERENCE_VALUE_POS = 5;
    static constexpr int GATEWAY_DEVICE_REFERENCE_TYPE_POS = 6;
    static constexpr int GATEWAY_DEVICE_REFERENCE_VALUE_POS = 7;
};
}    // namespace wolkabout

#endif
