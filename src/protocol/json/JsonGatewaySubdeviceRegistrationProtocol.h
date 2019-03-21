/*
 * Copyright 2019 WolkAbout Technology s.r.o.
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

class JsonGatewaySubdeviceRegistrationProtocol : public GatewaySubdeviceRegistrationProtocol
{
public:
    std::vector<std::string> getInboundChannels() const override;
    std::vector<std::string> getInboundChannelsForDevice(const std::string& deviceKey) const override;

    std::unique_ptr<Message> makeMessage(const SubdeviceRegistrationResponse& request) const override;

    std::unique_ptr<SubdeviceRegistrationRequest> makeSubdeviceRegistrationRequest(
      const Message& message) const override;

    bool isSubdeviceRegistrationRequest(const Message& message) const override;

private:
    static const std::string SUBDEVICE_REGISTRATION_REQUEST_TOPIC_ROOT;
    static const std::string SUBDEVICE_REGISTRATION_RESPONSE_TOPIC_ROOT;
};
}    // namespace wolkabout

#endif
