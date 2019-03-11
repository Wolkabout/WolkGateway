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
#ifndef GATEWAYPROTOCOL_H
#define GATEWAYPROTOCOL_H

#include <string>
#include <vector>

namespace wolkabout
{
class Message;

class GatewayProtocol
{
public:
    enum class Type
    {
        DATA,
        REGISTRATION,
        STATUS,
        FIRMWARE_UPDATE,
        FILE_DOWNLOAD
    };

    virtual ~GatewayProtocol() = default;

    virtual Type getType() const = 0;
    virtual const std::string& getName() const = 0;

    /**
     * @brief Get generic inbound device channels
     * @return
     */
    virtual std::vector<std::string> getInboundChannels() const = 0;

    /**
     * @brief Get inbound device channels for provided device key
     * @param deviceKey
     * @return
     */
    virtual std::vector<std::string> getInboundChannelsForDevice(const std::string& deviceKey) const = 0;

    virtual std::string extractDeviceKeyFromChannel(const std::string& topic) const = 0;

    virtual bool isMessageToPlatform(const Message& channel) const = 0;
    virtual bool isMessageFromPlatform(const Message& channel) const = 0;
};
}    // namespace wolkabout

#endif    // GATEWAYPROTOCOL_H
