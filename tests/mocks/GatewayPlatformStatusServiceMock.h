/**
 * Copyright 2022 Wolkabout Technology s.r.o.
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

#ifndef WOLKGATEWAY_GATEWAYPLATFORMSTATUSSERVICEMOCK_H
#define WOLKGATEWAY_GATEWAYPLATFORMSTATUSSERVICEMOCK_H

#include "gateway/service/platform_status/GatewayPlatformStatusService.h"

#include <gmock/gmock.h>

#include <utility>

using namespace wolkabout;
using namespace wolkabout::gateway;

class GatewayPlatformStatusServiceMock : public GatewayPlatformStatusService
{
public:
    GatewayPlatformStatusServiceMock(ConnectivityService& connectivityService, GatewayPlatformStatusProtocol& protocol,
                                     std::string deviceKey = "")
    : GatewayPlatformStatusService(connectivityService, protocol, std::move(deviceKey))
    {
    }
    MOCK_METHOD(void, sendPlatformConnectionStatusMessage, (bool));
};

#endif    // WOLKGATEWAY_GATEWAYPLATFORMSTATUSSERVICEMOCK_H
