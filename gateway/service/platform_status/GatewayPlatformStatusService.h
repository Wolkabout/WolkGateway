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

#ifndef PLATFORMSTATUSSERVICE_H
#define PLATFORMSTATUSSERVICE_H

#include <string>

namespace wolkabout
{
// Forward declare the service and the protocol from the SDK.
class ConnectivityService;
class GatewayPlatformStatusProtocol;

namespace gateway
{
class GatewayPlatformStatusService
{
public:
    GatewayPlatformStatusService(ConnectivityService& connectivityService, GatewayPlatformStatusProtocol& protocol,
                                 std::string deviceKey = "");

    virtual ~GatewayPlatformStatusService() = default;

    virtual void sendPlatformConnectionStatusMessage(bool connected);

private:
    ConnectivityService& m_connectivityService;
    GatewayPlatformStatusProtocol& m_protocol;

    std::string m_deviceKey;
};
}    // namespace gateway
}    // namespace wolkabout
#endif    // PLATFORMSTATUSSERVICE_H
