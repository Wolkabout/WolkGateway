/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#include "GatewayPlatformStatusService.h"

#include "core/connectivity/ConnectivityService.h"
#include "core/model/Message.h"
#include "core/protocol/GatewayPlatformStatusProtocol.h"
#include "core/utility/Logger.h"

using namespace wolkabout::legacy;

namespace wolkabout::gateway
{
GatewayPlatformStatusService::GatewayPlatformStatusService(ConnectivityService& connectivityService,
                                                           GatewayPlatformStatusProtocol& protocol,
                                                           std::string deviceKey)
: m_connectivityService(connectivityService), m_protocol(protocol), m_deviceKey(std::move(deviceKey))
{
}

void GatewayPlatformStatusService::sendPlatformConnectionStatusMessage(bool connected)
{
    LOG(TRACE) << METHOD_INFO;
    const auto errorPrefix = "Failed to send 'PlatformStatusMessage'";

    // Make the message
    auto status = connected ? ConnectivityStatus::CONNECTED : ConnectivityStatus::OFFLINE;
    auto message = std::shared_ptr<Message>{m_protocol.makeOutboundMessage(m_deviceKey, PlatformStatusMessage{status})};
    if (message == nullptr)
    {
        LOG(ERROR) << errorPrefix << " -> Failed to parse the outbound message.";
        return;
    }

    // Try to send out the message
    if (!m_connectivityService.publish(message))
        LOG(ERROR) << errorPrefix << " -> Failed to send the message.";
}
}    // namespace wolkabout::gateway
