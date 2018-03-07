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

    static bool isMessageToPlatform(const std::string& channel);
    static bool isMessageFromPlatform(const std::string& channel);

    static bool isStatusResponseMessage(const std::string& topic);
    static bool isStatusRequestMessage(const std::string& topic);
    static bool isLastWillMessage(const std::string& topic);

    static std::string routeDeviceMessage(const std::string& topic, const std::string& gatewayKey);
    static std::string routePlatformMessage(const std::string& topic, const std::string& gatewayKey);

    static std::string extractDeviceKeyFromChannel(const std::string& topic);
    static std::vector<std::string> deviceKeysFromContent(const std::string& content);

    static const std::string STATUS_RESPONSE_STATE_FIELD;
    static const std::string STATUS_RESPONSE_STATUS_CONNECTED;
    static const std::string STATUS_RESPONSE_STATUS_SLEEP;
    static const std::string STATUS_RESPONSE_STATUS_SERVICE;
    static const std::string STATUS_RESPONSE_STATUS_OFFLINE;

private:
    static const std::string NAME;

    static const std::string CHANNEL_DELIMITER;
    static const std::string CHANNEL_WILDCARD;
    static const std::string GATEWAY_PATH_PREFIX;
    static const std::string DEVICE_PATH_PREFIX;
    static const std::string DEVICE_TO_PLATFORM_DIRECTION;
    static const std::string PLATFORM_TO_DEVICE_DIRECTION;

    static const std::string LAST_WILL_TOPIC_ROOT;
    static const std::string DEVICE_STATUS_REQUEST_TOPIC_ROOT;
    static const std::string DEVICE_STATUS_RESPONSE_TOPIC_ROOT;

    static const std::vector<std::string> DEVICE_TOPICS;
    static const std::vector<std::string> PLATFORM_TOPICS;
};
}    // namespace wolkabout

#endif    // STATUSPROTOCOL_H
