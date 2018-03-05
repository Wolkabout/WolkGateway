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

#include "connectivity/Protocol.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class DeviceRegistrationRequestDto;
class DeviceRegistrationResponseDto;
class DeviceReregistrationResponseDto;

class DeviceRegistrationProtocol : public ProtocolBase<DeviceRegistrationProtocol>
{
    friend class ProtocolBase<DeviceRegistrationProtocol>;

public:
    std::vector<std::string> getDeviceTopics() override;
    std::vector<std::string> getPlatformTopics() override;

    std::shared_ptr<Message> makeMessage(const std::string& gatewayKey, const std::string& deviceKey,
                                         const DeviceRegistrationRequestDto& request);
    std::shared_ptr<Message> makeMessage(const std::string& gatewayKey, const std::string& deviceKey,
                                         const DeviceRegistrationResponseDto& response);
    std::shared_ptr<Message> makeMessage(const std::string& gatewayKey,
                                         const DeviceReregistrationResponseDto& response);

    std::shared_ptr<DeviceRegistrationRequestDto> makeRegistrationRequest(std::shared_ptr<Message> message);
    std::shared_ptr<DeviceRegistrationResponseDto> makeRegistrationResponse(std::shared_ptr<Message> message);

    bool isMessageToPlatform(const std::string& channel);
    bool isMessageFromPlatform(const std::string& channel);

    bool isRegistrationRequest(std::shared_ptr<Message> message);
    bool isRegistrationResponse(std::shared_ptr<Message> message);

    bool isReregistrationRequest(std::shared_ptr<Message> message);
    bool isReregistrationResponse(std::shared_ptr<Message> message);

    std::string extractDeviceKeyFromChannel(const std::string& channel);

private:
    DeviceRegistrationProtocol();

    const std::vector<std::string> m_deviceTopics;
    const std::vector<std::string> m_platformTopics;

    static constexpr const char* REGISTRATION_RESPONSE_OK = "OK";
    static constexpr const char* REGISTRATION_RESPONSE_ERROR_KEY_CONFLICT = "ERROR_KEY_CONFLICT";
    static constexpr const char* REGISTRATION_RESPONSE_ERROR_MANIFEST_CONFLICT = "ERROR_MANIFEST_CONFLICT";
    static constexpr const char* REGISTRATION_RESPONSE_ERROR_MAX_NUMBER_OF_DEVICES_EXCEEDED =
      "ERROR_MAXIMUM_NUMBER_OF_DEVICES_EXCEEDED";
    static constexpr const char* REGISTRATION_RESPONSE_ERROR_READING_PAYLOAD = "ERROR_READING_PAYLOAD";
    static constexpr const char* REGISTRATION_RESPONSE_ERROR_GATEWAY_NOT_FOUND = "ERROR_GATEWAY_NOT_FOUND";
    static constexpr const char* REGISTRATION_RESPONSE_ERROR_NO_GATEWAY_MANIFEST = "ERROR_NO_GATEWAY_MANIFEST";
};
}    // namespace wolkabout

#endif
