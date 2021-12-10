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

#include "PlatformStatusService.h"

#include "core/connectivity/ConnectivityService.h"
#include "core/model/Message.h"
#include "core/utilities/Logger.h"
#include "protocol/GatewayStatusProtocol.h"

namespace wolkabout
{

PlatformStatusService::PlatformStatusService(ConnectivityService& connectivityService, GatewayStatusProtocol& protocol)
: m_connectivityService(connectivityService), m_protocol(protocol)
{
}

void PlatformStatusService::sendPlatformConnectionStatusMessage(bool connected)
{
    std::shared_ptr<Message> message = m_protocol.makePlatformConnectionStatusMessage(connected);
    if(!message)
    {
        return;
    }

    if(!m_connectivityService.publish(message, true))
    {
        LOG(DEBUG) << "PlatformStatusService: Failed to send platform status message";
    }
    else
    {
        LOG(DEBUG) << "PlatformStatusService: Published platform status message";
    }
}

}   // namespace wolkabout
