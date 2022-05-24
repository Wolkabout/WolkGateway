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

#include <any>
#include <sstream>

#define private public
#define protected public
#include "gateway/service/platform_status/GatewayPlatformStatusService.h"
#undef private
#undef protected

#include "core/utilities/Logger.h"
#include "tests/mocks/ConnectivityServiceMock.h"
#include "tests/mocks/GatewayPlatformStatusProtocolMock.h"

#include <gtest/gtest.h>

using namespace wolkabout;
using namespace wolkabout::gateway;
using namespace ::testing;

class GatewayPlatformStatusServiceTests : public Test
{
public:
    static void SetUpTestCase() { Logger::init(LogLevel::TRACE, Logger::Type::CONSOLE); }

    void SetUp() override
    {
        service = std::unique_ptr<GatewayPlatformStatusService>{new GatewayPlatformStatusService{
          m_connectivityServiceMock, m_gatewayPlatformStatusProtocolMock, GATEWAY_KEY}};
    }

    std::unique_ptr<GatewayPlatformStatusService> service;

    const std::string GATEWAY_KEY = "TEST_GATEWAY";

    NiceMock<ConnectivityServiceMock> m_connectivityServiceMock;
    NiceMock<GatewayPlatformStatusProtocolMock> m_gatewayPlatformStatusProtocolMock;
};

TEST_F(GatewayPlatformStatusServiceTests, PublishStatusProtocolDidntParseMessage)
{
    EXPECT_CALL(m_gatewayPlatformStatusProtocolMock, makeOutboundMessage).WillOnce(Return(ByMove(nullptr)));
    ASSERT_NO_FATAL_FAILURE(service->sendPlatformConnectionStatusMessage(true));
}

TEST_F(GatewayPlatformStatusServiceTests, PublishStatusConnectivityServiceRefusesToSend)
{
    EXPECT_CALL(m_gatewayPlatformStatusProtocolMock, makeOutboundMessage)
      .WillOnce(Return(
        ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"CONNECTED", "p2d/connection_status"}})));
    EXPECT_CALL(m_connectivityServiceMock, publish).WillOnce(Return(false));
    ASSERT_NO_FATAL_FAILURE(service->sendPlatformConnectionStatusMessage(true));
}

TEST_F(GatewayPlatformStatusServiceTests, PublishStatusHappyFlow)
{
    EXPECT_CALL(m_gatewayPlatformStatusProtocolMock, makeOutboundMessage)
      .WillOnce(Return(
        ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"CONNECTED", "p2d/connection_status"}})));
    EXPECT_CALL(m_connectivityServiceMock, publish).WillOnce(Return(true));
    ASSERT_NO_FATAL_FAILURE(service->sendPlatformConnectionStatusMessage(true));
}
