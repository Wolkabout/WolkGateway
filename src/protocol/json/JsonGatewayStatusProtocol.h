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

#ifndef JSONGATEWAYSTATUSPROTOCOL_H
#define JSONGATEWAYSTATUSPROTOCOL_H

#include "protocol/GatewayStatusProtocol.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;

class JsonGatewayStatusProtocol : public GatewayStatusProtocol
{
public:
    std::vector<std::string> getInboundChannels() const override;
    std::vector<std::string> getInboundChannelsForDevice(const std::string& deviceKey) const override;
    std::string extractDeviceKeyFromChannel(const std::string& topic) const override;

    std::unique_ptr<Message> makeDeviceStatusRequestMessage(const std::string& deviceKey) const override;

    std::unique_ptr<DeviceStatus> makeDeviceStatusResponse(const Message& message) const override;
    std::unique_ptr<DeviceStatus> makeDeviceStatusUpdate(const Message& message) const override;

    bool isStatusResponseMessage(const Message& message) const override;
    bool isStatusUpdateMessage(const Message& message) const override;

    bool isLastWillMessage(const Message& message) const override;

    std::vector<std::string> extractDeviceKeysFromContent(const std::string& content) const override;

    static const std::string STATUS_RESPONSE_STATE_FIELD;
    static const std::string STATUS_RESPONSE_STATUS_CONNECTED;
    static const std::string STATUS_RESPONSE_STATUS_SLEEP;
    static const std::string STATUS_RESPONSE_STATUS_SERVICE;
    static const std::string STATUS_RESPONSE_STATUS_OFFLINE;

private:
    static const std::string LAST_WILL_TOPIC_ROOT;
    static const std::string DEVICE_STATUS_RESPONSE_TOPIC_ROOT;
    static const std::string DEVICE_STATUS_UPDATE_TOPIC_ROOT;
    static const std::string DEVICE_STATUS_REQUEST_TOPIC_ROOT;
};
}    // namespace wolkabout

#endif    // STATUSPROTOCOL_H
