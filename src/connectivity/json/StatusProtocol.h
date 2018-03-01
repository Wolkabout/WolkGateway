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

#include "connectivity/Protocol.h"
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class DeviceStatusResponse;

class StatusProtocol : public ProtocolBase<StatusProtocol>
{
public:
    std::vector<std::string> getDeviceTopics() override;
    std::vector<std::string> getPlatformTopics() override;

    std::shared_ptr<Message> make(const std::string& gatewayKey, const std::string& deviceKey,
                                  std::shared_ptr<DeviceStatusResponse> response);

    std::shared_ptr<Message> messageFromDeviceStatusRequest(const std::string& deviceKey);

    std::shared_ptr<DeviceStatusResponse> makeDeviceStatusResponse(std::shared_ptr<Message> message);

    bool isGatewayToPlatformMessage(const std::string& topic);

    bool isDeviceToPlatformMessage(const std::string& topic);

    bool isPlatformToDeviceMessage(const std::string& topic);

    bool isStatusResponseMessage(const std::string& topic);
    bool isStatusRequestMessage(const std::string& topic);
    bool isLastWillMessage(const std::string& topic);

    std::string routeDeviceMessage(const std::string& topic, const std::string& gatewayKey);
    std::string routePlatformMessage(const std::string& topic, const std::string& gatewayKey);

    std::string deviceKeyFromTopic(const std::string& topic);

    static const std::string STATUS_RESPONSE_STATE_FIELD;
    static const std::string STATUS_RESPONSE_STATUS_CONNECTED;
    static const std::string STATUS_RESPONSE_STATUS_SLEEP;
    static const std::string STATUS_RESPONSE_STATUS_SERVICE;
    static const std::string STATUS_RESPONSE_STATUS_OFFLINE;

private:
    friend class ProtocolBase<StatusProtocol>;

    StatusProtocol();

    const std::vector<std::string> m_deviceTopics;
    const std::vector<std::string> m_platformTopics;

    const std::vector<std::string> m_deviceMessageTypes;
    const std::vector<std::string> m_platformMessageTypes;

    static constexpr int DIRRECTION_POS = 0;
    static constexpr int TYPE_POS = 1;
    static constexpr int GATEWAY_TYPE_POS = 2;
    static constexpr int GATEWAY_KEY_POS = 3;
    static constexpr int DEVICE_TYPE_POS = 2;
    static constexpr int DEVICE_KEY_POS = 3;
    static constexpr int GATEWAY_DEVICE_TYPE_POS = 4;
    static constexpr int GATEWAY_DEVICE_KEY_POS = 5;
};
}

#endif    // STATUSPROTOCOL_H
