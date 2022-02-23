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
#include "gateway/WolkGateway.h"
#undef private
#undef protected

#include "core/utilities/Logger.h"
#include "tests/mocks/ConnectivityServiceMock.h"
#include "tests/mocks/DataProtocolMock.h"
#include "tests/mocks/DataServiceMock.h"
#include "tests/mocks/DevicesServiceMock.h"
#include "tests/mocks/GatewayPlatformStatusProtocolMock.h"
#include "tests/mocks/GatewayPlatformStatusServiceMock.h"
#include "tests/mocks/OutboundMessageHandlerMock.h"
#include "tests/mocks/OutboundRetryMessageHandlerMock.h"
#include "tests/mocks/PersistenceMock.h"
#include "tests/mocks/RegistrationProtocolMock.h"

#include <gtest/gtest.h>

using namespace wolkabout;
using namespace wolkabout::connect;
using namespace wolkabout::gateway;
using namespace ::testing;

class WolkGatewayTests : public Test
{
public:
    static void SetUpTestCase() { Logger::init(LogLevel::TRACE, Logger::Type::CONSOLE); }

    void SetUp() override
    {
        dataServiceMock = std::unique_ptr<DataServiceMock>{
          new DataServiceMock{dataProtocolMock, persistenceMock, connectivityServiceMock,
                              [](const std::string&, const std::map<std::uint64_t, std::vector<Reading>>&) {},
                              [](const std::string&, const std::vector<Parameter>&) {}}};
        devicesServiceMock = std::unique_ptr<DevicesServiceMock>{new DevicesServiceMock{
          gateway.getKey(), registrationProtocolMock, outboundMessageHandlerMock, outboundRetryMessageHandlerMock}};
        gatewayPlatformStatusServiceMock = std::unique_ptr<GatewayPlatformStatusServiceMock>{
          new GatewayPlatformStatusServiceMock{connectivityServiceMock, gatewayPlatformStatusProtocolMock}};
        service = std::unique_ptr<WolkGateway>{new WolkGateway{gateway}};
    }

    const Device gateway{"TestGateway", "TestPassword", OutboundDataMode::PUSH};

    std::unique_ptr<WolkGateway> service;

    DataProtocolMock dataProtocolMock;

    PersistenceMock persistenceMock;

    ConnectivityServiceMock connectivityServiceMock;

    std::unique_ptr<DataServiceMock> dataServiceMock;

    RegistrationProtocolMock registrationProtocolMock;

    OutboundMessageHandlerMock outboundMessageHandlerMock;

    OutboundRetryMessageHandlerMock outboundRetryMessageHandlerMock{outboundMessageHandlerMock};

    std::unique_ptr<DevicesServiceMock> devicesServiceMock;

    GatewayPlatformStatusProtocolMock gatewayPlatformStatusProtocolMock;

    std::unique_ptr<GatewayPlatformStatusServiceMock> gatewayPlatformStatusServiceMock;
};

TEST_F(WolkGatewayTests, NewBuilder)
{
    ASSERT_NO_FATAL_FAILURE(WolkGateway::newBuilder(gateway));
}

TEST_F(WolkGatewayTests, ReturnsFalseForLocalConnectionStatusBecauseItDoesntExist)
{
    EXPECT_FALSE(service->isLocalConnected());
}

TEST_F(WolkGatewayTests, CheckTheTypeOfWolkInterface)
{
    EXPECT_EQ(service->getType(), WolkInterfaceType::Gateway);
}

TEST_F(WolkGatewayTests, CheckTheRTC)
{
    EXPECT_GT(service->currentRtc(), 0);
}

TEST_F(WolkGatewayTests, NothingExplodesIfBothAreNull)
{
    service->connect();
    std::this_thread::sleep_for(std::chrono::milliseconds{100});
}

TEST_F(WolkGatewayTests, RepeatMechanisms)
{
    // Set up the services
    EXPECT_CALL(*dataServiceMock, publishReadings()).Times(1);
    EXPECT_CALL(*dataServiceMock, publishAttributes()).Times(1);
    EXPECT_CALL(*dataServiceMock, publishParameters()).Times(1);
    service->m_dataService = std::move(dataServiceMock);

    // Set up the connectivity service mocks
    auto platformConnectivityService = std::unique_ptr<ConnectivityServiceMock>{new NiceMock<ConnectivityServiceMock>};
    auto localConnectivityService = std::make_shared<NiceMock<ConnectivityServiceMock>>();
    std::atomic_bool platformConnected{false};
    std::atomic_bool localConnected{false};
    EXPECT_CALL(*platformConnectivityService, connect).WillOnce(Return(false)).WillOnce([&]() {
        platformConnected = true;
        return true;
    });
    EXPECT_CALL(*localConnectivityService, connect).WillOnce(Return(false)).WillOnce([&]() {
        localConnected = true;
        return true;
    });
    service->m_connectivityService = std::move(platformConnectivityService);
    service->m_localConnectivityService = std::move(localConnectivityService);

    // Connect
    ASSERT_NO_FATAL_FAILURE(service->connect());
    std::this_thread::sleep_for(std::chrono::seconds(2) * 5);    // At least 4 * RECONNECT_DELAY_MSEC
    EXPECT_TRUE(platformConnected);
    EXPECT_TRUE(localConnected);
}

TEST_F(WolkGatewayTests, ConnectHappyFlow)
{
    // Make two connectivity service mocks and inject them
    auto platformConnectivityService = std::unique_ptr<ConnectivityServiceMock>{new NiceMock<ConnectivityServiceMock>};
    auto localConnectivityService = std::make_shared<NiceMock<ConnectivityServiceMock>>();
    std::atomic_bool platformConnected{false};
    std::atomic_bool localConnected{false};
    EXPECT_CALL(*platformConnectivityService, connect)
      .Times(AtLeast(1))
      .WillOnce([&]() {
          platformConnected = true;
          return true;
      })
      .WillRepeatedly(Return(false));
    EXPECT_CALL(*platformConnectivityService, disconnect).WillOnce([&]() {
        service->platformDisconnected();
        platformConnected = false;
        return true;
    });
    EXPECT_CALL(*localConnectivityService, connect).WillOnce([&]() {
        localConnected = true;
        return true;
    });
    EXPECT_CALL(*localConnectivityService, disconnect).Times(1);
    service->m_connectivityService = std::move(platformConnectivityService);
    service->m_localConnectivityService = std::move(localConnectivityService);

    // Set up the services
    EXPECT_CALL(*dataServiceMock, publishReadings()).Times(1);
    EXPECT_CALL(*dataServiceMock, publishAttributes()).Times(1);
    EXPECT_CALL(*dataServiceMock, publishParameters()).Times(1);
    EXPECT_CALL(*devicesServiceMock, updateDeviceCache).Times(1);
    EXPECT_CALL(*gatewayPlatformStatusServiceMock, sendPlatformConnectionStatusMessage).Times(2);
    service->m_dataService = std::move(dataServiceMock);
    service->m_subdeviceManagementService = std::move(devicesServiceMock);
    service->m_gatewayPlatformStatusService = std::move(gatewayPlatformStatusServiceMock);

    // Check the connection statuses
    EXPECT_FALSE(service->isPlatformConnected());
    EXPECT_FALSE(service->isLocalConnected());

    // Set up the expected calls
    std::mutex mutex;
    std::condition_variable conditionVariable;
    service->setConnectionStatusListener([&](bool connected) {
        if (connected)
            conditionVariable.notify_one();
    });

    // And invoke connect
    service->connect();
    if (!platformConnected && !localConnected)
    {
        std::unique_lock<std::mutex> lock{mutex};
        conditionVariable.wait_for(lock, std::chrono::milliseconds{100},
                                   [&]() { return platformConnected && localConnected; });
    }
    EXPECT_TRUE(platformConnected);
    EXPECT_TRUE(localConnected);
    EXPECT_TRUE(service->isPlatformConnected());
    EXPECT_TRUE(service->isLocalConnected());

    service->disconnect();
    if (platformConnected)
    {
        std::unique_lock<std::mutex> lock{mutex};
        conditionVariable.wait_for(lock, std::chrono::milliseconds{100}, [&]() { return !platformConnected; });
    }
    EXPECT_FALSE(platformConnected);
    EXPECT_FALSE(service->isPlatformConnected());
    EXPECT_FALSE(service->isLocalConnected());
}
