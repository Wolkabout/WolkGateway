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

#ifndef STATUSPROTOCOL_H
#define STATUSPROTOCOL_H

#include "model/DeviceStatusResponse.h"
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;

class StatusProtocol
{
public:
    ~StatusProtocol() = delete;

    static const std::string& getName();

    static const std::vector<std::string>& getDeviceTopics();
    static const std::vector<std::string>& getPlatformTopics();

    static std::shared_ptr<Message> messageFromDeviceStatusResponse(const std::string& gatewayKey,
                                                                    const std::string& deviceKey,
                                                                    std::shared_ptr<DeviceStatusResponse> response);

    static std::shared_ptr<Message> messageFromDeviceStatusResponse(const std::string& gatewayKey,
                                                                    const std::string& deviceKey,
                                                                    const DeviceStatusResponse::Status& response);

    static std::shared_ptr<Message> messageFromDeviceStatusRequest(const std::string& deviceKey);

    static std::shared_ptr<DeviceStatusResponse> makeDeviceStatusResponse(std::shared_ptr<Message> message);

    static bool isGatewayToPlatformMessage(const std::string& topic);

    static bool isDeviceToPlatformMessage(const std::string& topic);

    static bool isPlatformToDeviceMessage(const std::string& topic);

    static bool isStatusResponseMessage(const std::string& topic);
    static bool isStatusRequestMessage(const std::string& topic);
    static bool isLastWillMessage(const std::string& topic);

    static std::string routeDeviceMessage(const std::string& topic, const std::string& gatewayKey);
    static std::string routePlatformMessage(const std::string& topic, const std::string& gatewayKey);

    static std::string deviceKeyFromTopic(const std::string& topic);
    static std::string gatewayKeyFromTopic(const std::string& topic);
    static std::vector<std::string> deviceKeysFromContent(const std::string& content);

    static const std::string STATUS_RESPONSE_STATE_FIELD;
    static const std::string STATUS_RESPONSE_STATUS_CONNECTED;
    static const std::string STATUS_RESPONSE_STATUS_SLEEP;
    static const std::string STATUS_RESPONSE_STATUS_SERVICE;
    static const std::string STATUS_RESPONSE_STATUS_OFFLINE;

private:
    static const std::string m_name;

    static const std::vector<std::string> m_deviceTopics;
    static const std::vector<std::string> m_platformTopics;

    static const std::vector<std::string> m_deviceMessageTypes;
    static const std::vector<std::string> m_platformMessageTypes;

    static constexpr int DIRRECTION_POS = 0;
    static constexpr int TYPE_POS = 1;
    static constexpr int GATEWAY_TYPE_POS = 2;
    static constexpr int GATEWAY_KEY_POS = 3;
    static constexpr int DEVICE_TYPE_POS = 2;
    static constexpr int DEVICE_KEY_POS = 3;
    static constexpr int GATEWAY_DEVICE_TYPE_POS = 4;
    static constexpr int GATEWAY_DEVICE_KEY_POS = 5;
};
}    // namespace wolkabout

#endif    // STATUSPROTOCOL_H
