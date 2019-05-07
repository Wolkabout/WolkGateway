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
#include "model/SubdeviceManagement.h"
#include "protocol/json/JsonDFUProtocol.h"
#include "protocol/json/JsonDownloadProtocol.h"
#include "protocol/json/JsonGatewayDFUProtocol.h"
#include "protocol/json/JsonGatewayDataProtocol.h"
#include "protocol/json/JsonGatewaySubdeviceRegistrationProtocol.h"
#include "protocol/json/JsonProtocol.h"
#include "protocol/json/JsonRegistrationProtocol.h"
#include "service/DataService.h"
#include "service/FileDownloadService.h"
#include "service/FirmwareUpdateService.h"
#include "service/GatewayUpdateService.h"
#include "service/KeepAliveService.h"
#include "service/PublishingService.h"
#include "service/SubdeviceRegistrationService.h"

#include <memory>
#include <protocol/json/JsonStatusProtocol.h>

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

    MOCK_METHOD1(requestActuatorStatusesForDevice, void(const std::string&));
    MOCK_METHOD0(requestActuatorStatusesForAllDevices, void());

private:
    GTEST_DISALLOW_COPY_AND_ASSIGN_(MockDataService);
};

class MockGatewayUpdateService : public wolkabout::GatewayUpdateService
{
public:
    using wolkabout::GatewayUpdateService::GatewayUpdateService;

    MOCK_METHOD1(updateGateway, void(const wolkabout::DetailedDevice&));
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

class MockFileDownloadService : public wolkabout::FileDownloadService
{
public:
    using wolkabout::FileDownloadService::FileDownloadService;

    MOCK_METHOD0(sendFileList, void());
};

class MockFirmwareUpdateService : public wolkabout::FirmwareUpdateService
{
public:
    using wolkabout::FirmwareUpdateService::FirmwareUpdateService;

    MOCK_METHOD0(reportFirmwareUpdateResult, void());
    MOCK_METHOD0(publishFirmwareVersion, void());
};

class MockKeepAliveService : public wolkabout::KeepAliveService
{
public:
    using wolkabout::KeepAliveService::KeepAliveService;

    MOCK_CONST_METHOD0(sendPingMessage, void());
};

class MockSubdeviceRegistrationService : public wolkabout::SubdeviceRegistrationService
{
public:
    using wolkabout::SubdeviceRegistrationService::SubdeviceRegistrationService;

    MOCK_METHOD0(registerPostponedDevices, void());
    MOCK_METHOD1(deleteDevicesOtherThan, void(const std::vector<std::string>&));
};

class Wolk : public ::testing::Test
{
public:
    void SetUp(wolkabout::SubdeviceManagement control)
    {
        platformConnectivityService = new MockConnectivityService();
        deviceConnectivityService = new MockConnectivityService();

        wolk = std::unique_ptr<wolkabout::Wolk>(
          new wolkabout::Wolk(wolkabout::GatewayDevice{GATEWAY_KEY, "password", control, true, true}));
        wolk->m_platformConnectivityService.reset(platformConnectivityService);
        wolk->m_deviceConnectivityService.reset(deviceConnectivityService);
        wolk->m_platformPublisher.reset(new Publisher(*platformConnectivityService, nullptr));
        wolk->m_devicePublisher.reset(new Publisher(*deviceConnectivityService, nullptr));
        wolk->m_inboundPlatformMessageHandler.reset(new GatewayInboundPlatformMessageHandler());
        wolk->m_inboundDeviceMessageHandler.reset(new GatewayInboundDeviceMessageHandler());

        deviceRepository = new MockRepository();
        wolk->m_deviceRepository.reset(deviceRepository);
        fileRepository = new MockFileRepository();
        wolk->m_fileRepository.reset(fileRepository);
        existingDevicesRepository = new MockExistingDevicesRepository();
        wolk->m_existingDevicesRepository.reset(existingDevicesRepository);

        dataProtocol = std::make_shared<wolkabout::JsonProtocol>(true);
        gatewayDataProtocol = std::make_shared<wolkabout::JsonGatewayDataProtocol>();
        deviceRegistrationProtocol = std::make_shared<wolkabout::JsonRegistrationProtocol>();
        fileDownloadProtocol = std::make_shared<wolkabout::JsonDownloadProtocol>(true);
        firmwareUpdateProtocol = std::make_shared<wolkabout::JsonDFUProtocol>(true);
        gatewayFirmwareUpdateProtocol = std::make_shared<wolkabout::JsonGatewayDFUProtocol>();
        statusProtocol = std::make_shared<wolkabout::JsonStatusProtocol>(true);
        gatewayRegistrationProtocol = std::make_shared<wolkabout::JsonGatewaySubdeviceRegistrationProtocol>();

        dataService =
          new MockDataService(GATEWAY_KEY, *dataProtocol, *gatewayDataProtocol, wolk->m_deviceRepository.get(),
                              *wolk->m_platformPublisher, *wolk->m_devicePublisher);
        wolk->m_dataService.reset(dataService);

        gatewayUpdateService = new MockGatewayUpdateService(GATEWAY_KEY, *deviceRegistrationProtocol,
                                                            *wolk->m_deviceRepository, *wolk->m_platformPublisher);
        wolk->m_gatewayUpdateService.reset(gatewayUpdateService);

        fileDownloadService = new MockFileDownloadService(GATEWAY_KEY, *fileDownloadProtocol, "",
                                                          *wolk->m_platformPublisher, *wolk->m_fileRepository);
        wolk->m_fileDownloadService.reset(fileDownloadService);

        firmwareUpdateService =
          new MockFirmwareUpdateService(GATEWAY_KEY, *firmwareUpdateProtocol, *gatewayFirmwareUpdateProtocol,
                                        *wolk->m_fileRepository, *wolk->m_platformPublisher, *wolk->m_devicePublisher);
        wolk->m_firmwareUpdateService.reset(firmwareUpdateService);

        keepAliveService =
          new MockKeepAliveService(GATEWAY_KEY, *statusProtocol, *wolk->m_platformPublisher, std::chrono::seconds(30));
        wolk->m_keepAliveService.reset(keepAliveService);

        subdeviceRegistrationService = new MockSubdeviceRegistrationService(
          GATEWAY_KEY, *deviceRegistrationProtocol, *gatewayRegistrationProtocol, *wolk->m_deviceRepository,
          *wolk->m_platformPublisher, *wolk->m_devicePublisher);
        wolk->m_subdeviceRegistrationService.reset(subdeviceRegistrationService);
    }

    void TearDown() override {}

    MockRepository* deviceRepository;
    MockFileRepository* fileRepository;
    MockExistingDevicesRepository* existingDevicesRepository;

    MockConnectivityService* platformConnectivityService;
    MockConnectivityService* deviceConnectivityService;

    MockGatewayUpdateService* gatewayUpdateService;
    MockDataService* dataService;
    MockFileDownloadService* fileDownloadService;
    MockFirmwareUpdateService* firmwareUpdateService;
    MockKeepAliveService* keepAliveService;
    MockSubdeviceRegistrationService* subdeviceRegistrationService;

    std::shared_ptr<wolkabout::JsonProtocol> dataProtocol;
    std::shared_ptr<wolkabout::JsonGatewayDataProtocol> gatewayDataProtocol;
    std::shared_ptr<wolkabout::RegistrationProtocol> deviceRegistrationProtocol;
    std::shared_ptr<wolkabout::JsonDownloadProtocol> fileDownloadProtocol;
    std::shared_ptr<wolkabout::JsonDFUProtocol> firmwareUpdateProtocol;
    std::shared_ptr<wolkabout::JsonGatewayDFUProtocol> gatewayFirmwareUpdateProtocol;
    std::shared_ptr<wolkabout::StatusProtocol> statusProtocol;
    std::shared_ptr<wolkabout::GatewaySubdeviceRegistrationProtocol> gatewayRegistrationProtocol;

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

    void wait(int sec = 3)
    {
        std::unique_lock<std::mutex> lock{mutex};
        EXPECT_TRUE(cv.wait_for(lock, std::chrono::seconds(sec), [this] { return done; }));
    }
};
}    // namespace

TEST_F(
  Wolk,
  GivenGatewayInControlAndNoDeviceInRepository_When_ConnectingToPlatformIsSuccessful_Then_NoActuatorStatusRequestIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    const std::string deviceKey = "KEY1";

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy()).WillByDefault(testing::ReturnNew<std::vector<std::string>>());
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey)).WillByDefault(testing::Return(nullptr));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*dataService, requestActuatorStatusesForDevice(testing::_)).Times(0);
}

TEST_F(
  Wolk,
  GivenGatewayInControlAndGatewayAndNoDeviceInRepository_When_ConnectingToPlatformIsSuccessful_Then_NoActuatorStatusRequestIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    const std::string deviceKey = "KEY1";

    std::vector<std::string> keys = {GATEWAY_KEY};

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::ReturnNew<std::vector<std::string>>(keys));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey)).WillByDefault(testing::Return(nullptr));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*dataService, requestActuatorStatusesForDevice(testing::_)).Times(0);
}

TEST_F(
  Wolk,
  GivenGatewayInControlAndSingleDeviceInRepository_When_ConnectingToPlatformIsSuccessful_Then_ActuatorStatusRequestIsSentForDevice)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    const std::string deviceKey = "KEY1";

    std::vector<std::string> keys = {deviceKey};

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::ReturnNew<std::vector<std::string>>(keys));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey))
      .WillByDefault(testing::ReturnNew<wolkabout::DetailedDevice>(
        "", deviceKey,
        wolkabout::DeviceTemplate{{},
                                  {wolkabout::SensorTemplate{"", "REF", wolkabout::DataType::NUMERIC, "", {0}, {100}}},
                                  {},
                                  {},
                                  "",
                                  {},
                                  {},
                                  {}}));

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
  GivenGatewayInControlAndGatewayAndSingleDeviceInRepository_When_ConnectingToPlatformIsSuccessful_Then_ActuatorStatusRequestIsSentForDevice)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    const std::string deviceKey = "KEY1";

    std::vector<std::string> keys = {GATEWAY_KEY, deviceKey};

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::ReturnNew<std::vector<std::string>>(keys));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey))
      .WillByDefault(testing::ReturnNew<wolkabout::DetailedDevice>(
        "", deviceKey,
        wolkabout::DeviceTemplate{{},
                                  {wolkabout::SensorTemplate{"", "REF", wolkabout::DataType::NUMERIC, "", {0}, {100}}},
                                  {},
                                  {},
                                  "",
                                  {},
                                  {},
                                  {}}));

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
  GivenGatewayInControlAndGatewayAndMultipleDeviceInRepository_When_ConnectingToPlatformIsSuccessful_Then_ActuatorStatusRequestIsSentForEachDevice)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    const std::string deviceKey1 = "KEY1";
    const std::string deviceKey2 = "KEY2";
    const std::string deviceKey3 = "KEY3";

    std::vector<std::string> keys = {GATEWAY_KEY, deviceKey1, deviceKey2, deviceKey3};

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::ReturnNew<std::vector<std::string>>(keys));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey1))
      .WillByDefault(testing::ReturnNew<wolkabout::DetailedDevice>(
        "", deviceKey1,
        wolkabout::DeviceTemplate{{},
                                  {wolkabout::SensorTemplate{"", "REF", wolkabout::DataType::NUMERIC, "", {0}, {100}}},
                                  {},
                                  {},
                                  "",
                                  {},
                                  {},
                                  {}}));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey2))
      .WillByDefault(testing::ReturnNew<wolkabout::DetailedDevice>(
        "", deviceKey2,
        wolkabout::DeviceTemplate{{},
                                  {wolkabout::SensorTemplate{"", "REF", wolkabout::DataType::NUMERIC, "", {0}, {100}}},
                                  {},
                                  {},
                                  "",
                                  {},
                                  {},
                                  {}}));
    ON_CALL(*deviceRepository, findByDeviceKeyProxy(deviceKey3))
      .WillByDefault(testing::ReturnNew<wolkabout::DetailedDevice>(
        "", deviceKey2,
        wolkabout::DeviceTemplate{{},
                                  {wolkabout::SensorTemplate{"", "REF", wolkabout::DataType::NUMERIC, "", {0}, {100}}},
                                  {},
                                  {},
                                  "",
                                  {},
                                  {},
                                  {}}));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*dataService, requestActuatorStatusesForDevice(testing::_))
      .Times(3)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenGatewayInControl_When_ConnectingToPlatformIsSuccessful_Then_FileListIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*fileDownloadService, sendFileList())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenPlatformInControl_When_ConnectingToPlatformIsSuccessful_Then_FileListIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::PLATFORM);

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*fileDownloadService, sendFileList())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenGatewayInControl_When_ConnectingToPlatformIsSuccessful_Then_FirmwareStatusIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*firmwareUpdateService, reportFirmwareUpdateResult()).Times(1);

    EXPECT_CALL(*firmwareUpdateService, publishFirmwareVersion())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenPlatformInControl_When_ConnectingToPlatformIsSuccessful_Then_FirmwareStatusIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::PLATFORM);

    ON_CALL(*platformConnectivityService, connect()).WillByDefault(testing::Return(true));

    // When
    wolk->connectToPlatform();

    // Then
    EXPECT_CALL(*firmwareUpdateService, reportFirmwareUpdateResult()).Times(1);

    EXPECT_CALL(*firmwareUpdateService, publishFirmwareVersion())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenGatewayInControl_When_GatewayIsUpdated_Then_FileListIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    // When
    wolk->gatewayUpdated();

    // Then
    EXPECT_CALL(*fileDownloadService, sendFileList())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenPlatformInControl_When_GatewayIsUpdated_Then_FileListIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::PLATFORM);

    // When
    wolk->gatewayUpdated();

    // Then
    EXPECT_CALL(*fileDownloadService, sendFileList())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenGatewayInControl_When_GatewayIsUpdated_Then_FirmwareStatusIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    // When
    wolk->gatewayUpdated();

    // Then
    EXPECT_CALL(*firmwareUpdateService, reportFirmwareUpdateResult()).Times(1);

    EXPECT_CALL(*firmwareUpdateService, publishFirmwareVersion())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenPlatformInControl_When_GatewayIsUpdated_Then_FirmwareStatusIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::PLATFORM);

    // When
    wolk->gatewayUpdated();

    // Then
    EXPECT_CALL(*firmwareUpdateService, reportFirmwareUpdateResult()).Times(1);

    EXPECT_CALL(*firmwareUpdateService, publishFirmwareVersion())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenGatewayInControl_When_GatewayIsUpdated_Then_PingIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    // When
    wolk->gatewayUpdated();

    // Then
    EXPECT_CALL(*keepAliveService, sendPingMessage())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenPlatformInControl_When_GatewayIsUpdated_Then_PingIsSent)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::PLATFORM);

    // When
    wolk->gatewayUpdated();

    // Then
    EXPECT_CALL(*keepAliveService, sendPingMessage())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenGatewayInControl_When_GatewayIsUpdated_Then_PostponedDevicesAreRegistered)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::GATEWAY);

    // When
    wolk->gatewayUpdated();

    // Then
    EXPECT_CALL(*subdeviceRegistrationService, registerPostponedDevices())
      .Times(1)
      .WillOnce(testing::InvokeWithoutArgs(this, &Wolk::finished));

    wait();
}

TEST_F(Wolk, GivenPlatformInControl_When_GatewayIsUpdated_Then_PostponedDevicesAreNotRegistered)
{
    // Given
    SetUp(wolkabout::SubdeviceManagement::PLATFORM);

    // When
    wolk->gatewayUpdated();

    // Then
    EXPECT_CALL(*subdeviceRegistrationService, registerPostponedDevices()).Times(0);
}