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

#define private public
#define protected public
#include "Wolk.h"
#undef protected
#undef private

#include "GatewayInboundDeviceMessageHandler.h"
#include "GatewayInboundPlatformMessageHandler.h"
#include "MockConnectivityService.h"
#include "MockRepository.h"
#include "protocol/json/JsonGatewayDataProtocol.h"
#include "service/DataService.h"

#include <memory>

namespace
{
class Publisher : public wolkabout::PublishingService
{
public:
    using wolkabout::PublishingService::PublishingService;
    void addMessage(std::shared_ptr<wolkabout::Message>) override {}
    void connected() override {}
    void disconnected() override {}
};

class MockDataService : public wolkabout::DataService
{
public:
    using wolkabout::DataService::DataService;
    virtual ~MockDataService() {}

    MOCK_METHOD1(requestActuatorStatusesForDevice, void(const std::string&));

private:
    GTEST_DISALLOW_COPY_AND_ASSIGN_(MockDataService);
};

class GatewayInboundPlatformMessageHandler : public wolkabout::GatewayInboundPlatformMessageHandler
{
public:
    GatewayInboundPlatformMessageHandler() : wolkabout::GatewayInboundPlatformMessageHandler("") {}
    void messageReceived(const std::string&, const std::string&) override {}
    std::vector<std::string> getChannels() const override { return {}; }
    void addListener(std::weak_ptr<wolkabout::PlatformMessageListener>) override {}
};

class GatewayInboundDeviceMessageHandler : public wolkabout::GatewayInboundDeviceMessageHandler
{
public:
    void messageReceived(const std::string&, const std::string&) override {}
    std::vector<std::string> getChannels() const override { return {}; }
    void addListener(std::weak_ptr<wolkabout::DeviceMessageListener>) override {}
};

class Wolk : public ::testing::Test
{
public:
    void SetUp() override
    {
        platformConnectivityService = new MockConnectivityService();
        deviceConnectivityService = new MockConnectivityService();

        wolk = std::unique_ptr<wolkabout::Wolk>(
          new wolkabout::Wolk(wolkabout::Device{GATEWAY_KEY, "password", "JsonProtocol"}));
        wolk->m_platformConnectivityService.reset(platformConnectivityService);
        wolk->m_deviceConnectivityService.reset(deviceConnectivityService);
        wolk->m_platformPublisher.reset(new Publisher(*platformConnectivityService, nullptr));
        wolk->m_devicePublisher.reset(new Publisher(*deviceConnectivityService, nullptr));
        wolk->m_inboundPlatformMessageHandler.reset(new GatewayInboundPlatformMessageHandler);
        wolk->m_inboundDeviceMessageHandler.reset(new GatewayInboundDeviceMessageHandler);

        deviceRepository = new MockRepository();
        wolk->m_deviceRepository.reset(deviceRepository);

        dataProtocol = std::make_shared<wolkabout::JsonGatewayDataProtocol>();
        dataService = std::make_shared<MockDataService>("", *dataProtocol, *wolk->m_deviceRepository,
                                                        *wolk->m_platformPublisher, *wolk->m_devicePublisher);
        wolk->registerDataProtocol(dataProtocol, dataService);
    }

    void TearDown() override {}

    MockConnectivityService* platformConnectivityService;
    MockConnectivityService* deviceConnectivityService;

    std::shared_ptr<MockDataService> dataService;
    std::shared_ptr<wolkabout::JsonGatewayDataProtocol> dataProtocol;

    MockRepository* deviceRepository;

    std::unique_ptr<wolkabout::Wolk> wolk;

    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;

    const std::string GATEWAY_KEY = "gw_key";

    void finished()
    {
        std::lock_guard<std::mutex> lock{mutex};
        done = true;
        cv.notify_one();
    }

    void wait(int sec = 2)
    {
        std::unique_lock<std::mutex> lock{mutex};
        EXPECT_TRUE(cv.wait_for(lock, std::chrono::seconds(sec), [this] { return done; }));
    }
};
}    // namespace

TEST_F(Wolk, GivenNoDeviceInRepository_When_ConnectingToPlatformIsSuccessful_Then_NoActuatorStatusRequestIsSent)
{
    // Given
    const std::string deviceKey = "KEY1";

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy()).WillByDefault(testing::Return(new std::vector<std::string>{}));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey)).WillByDefault(testing::Return(nullptr));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*dataService, requestActuatorStatusesForDevice(testing::_)).Times(0);
}

TEST_F(Wolk,
       GivenGatewayAndNoDeviceInRepository_When_ConnectingToPlatformIsSuccessful_Then_NoActuatorStatusRequestIsSent)
{
    // Given
    const std::string deviceKey = "KEY1";

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::Return(new std::vector<std::string>{GATEWAY_KEY}));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey)).WillByDefault(testing::Return(nullptr));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*dataService, requestActuatorStatusesForDevice(testing::_)).Times(0);
}

TEST_F(Wolk,
       GivenSingleDeviceInRepository_When_ConnectingToPlatformIsSuccessful_Then_ActuatorStatusRequestIsSentForDevice)
{
    // Given
    const std::string deviceKey = "KEY1";

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::Return(new std::vector<std::string>{deviceKey}));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey))
      .WillByDefault(testing::Return(new wolkabout::DetailedDevice(
        "", deviceKey,
        wolkabout::DeviceManifest{
          "",
          "",
          dataProtocol->getName(),
          "",
          {},
          {wolkabout::SensorManifest{"", "REF", "", "", wolkabout::DataType::NUMERIC, 1, "", {}, 0, 100}},
          {},
          {}})));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*dataService, requestActuatorStatusesForDevice(deviceKey))
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(
  Wolk,
  GivenGatewayAndSingleDeviceInRepository_When_ConnectingToPlatformIsSuccessful_Then_ActuatorStatusRequestIsSentForDevice)
{
    // Given
    const std::string deviceKey = "KEY1";

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::Return(new std::vector<std::string>{GATEWAY_KEY, deviceKey}));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey))
      .WillByDefault(testing::Return(new wolkabout::DetailedDevice(
        "", deviceKey,
        wolkabout::DeviceManifest{
          "",
          "",
          dataProtocol->getName(),
          "",
          {},
          {wolkabout::SensorManifest{"", "REF", "", "", wolkabout::DataType::NUMERIC, 1, "", {}, 0, 100}},
          {},
          {}})));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*dataService, requestActuatorStatusesForDevice(deviceKey))
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(
  Wolk,
  GivenGatewayAndMultipleDeviceInRepository_When_ConnectingToPlatformIsSuccessful_Then_ActuatorStatusRequestIsSentForEachDevice)
{
    // Given
    const std::string deviceKey1 = "KEY1";
    const std::string deviceKey2 = "KEY2";
    const std::string deviceKey3 = "KEY3";

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::Return(new std::vector<std::string>{GATEWAY_KEY, deviceKey1, deviceKey2, deviceKey3}));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey1))
      .WillByDefault(testing::Return(new wolkabout::DetailedDevice(
        "", deviceKey1,
        wolkabout::DeviceManifest{
          "",
          "",
          dataProtocol->getName(),
          "",
          {},
          {wolkabout::SensorManifest{"", "REF", "", "", wolkabout::DataType::NUMERIC, 1, "", {}, 0, 100}},
          {},
          {}})));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey2))
      .WillByDefault(testing::Return(new wolkabout::DetailedDevice(
        "", deviceKey2,
        wolkabout::DeviceManifest{
          "",
          "",
          dataProtocol->getName(),
          "",
          {},
          {wolkabout::SensorManifest{"", "REF", "", "", wolkabout::DataType::NUMERIC, 1, "", {}, 0, 100}},
          {},
          {}})));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey3))
      .WillByDefault(testing::Return(new wolkabout::DetailedDevice(
        "", deviceKey2,
        wolkabout::DeviceManifest{
          "",
          "",
          dataProtocol->getName(),
          "",
          {},
          {wolkabout::SensorManifest{"", "REF", "", "", wolkabout::DataType::NUMERIC, 1, "", {}, 0, 100}},
          {},
          {}})));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*dataService, requestActuatorStatusesForDevice(testing::_))
      .Times(3)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}
