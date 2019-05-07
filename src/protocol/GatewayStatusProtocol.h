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

#ifndef GATEWAYSTATUSPROTOCOL_H
#define GATEWAYSTATUSPROTOCOL_H

#include "protocol/GatewayProtocol.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Message;
class DeviceStatus;

class GatewayStatusProtocol : public GatewayProtocol
{
public:
    using GatewayProtocol::extractDeviceKeyFromChannel;

    virtual std::unique_ptr<Message> makeDeviceStatusRequestMessage(const std::string& deviceKey) const = 0;

    virtual std::unique_ptr<DeviceStatus> makeDeviceStatusResponse(const Message& message) const = 0;
    virtual std::unique_ptr<DeviceStatus> makeDeviceStatusUpdate(const Message& message) const = 0;

    virtual bool isStatusResponseMessage(const Message& message) const = 0;
    virtual bool isStatusUpdateMessage(const Message& message) const = 0;

    virtual bool isLastWillMessage(const Message& message) const = 0;

    virtual std::vector<std::string> extractDeviceKeysFromContent(const std::string& content) const = 0;
};
}    // namespace wolkabout

#endif    // STATUSPROTOCOL_H
