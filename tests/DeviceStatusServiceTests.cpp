#include "ConnectionStatusListener.h"
#include "MockConnectionStatusListener.h"
#include "MockRepository.h"
#include "OutboundMessageHandler.h"
#include "model/Message.h"
#include "protocol/json/JsonGatewayStatusProtocol.h"
#include "protocol/json/JsonStatusProtocol.h"
#include "repository/SQLiteDeviceRepository.h"
#include "service/DeviceStatusService.h"

#include <gtest/gtest.h>
#include <cstdio>
#include <memory>
#include <vector>

namespace
{
class PlatformOutboundMessageHandler : public wolkabout::OutboundMessageHandler
{
public:
    void addMessage(std::shared_ptr<wolkabout::Message> message) override { m_messages.push_back(message); }

    const std::vector<std::shared_ptr<wolkabout::Message>>& getMessages() const { return m_messages; }

private:
    std::vector<std::shared_ptr<wolkabout::Message>> m_messages;
};

class DeviceOutboundMessageHandler : public wolkabout::OutboundMessageHandler
{
public:
    void addMessage(std::shared_ptr<wolkabout::Message> message) override { m_messages.push_back(message); }

    const std::vector<std::shared_ptr<wolkabout::Message>>& getMessages() const { return m_messages; }

private:
    std::vector<std::shared_ptr<wolkabout::Message>> m_messages;
};

class DeviceStatusService : public ::testing::Test
{
public:
    void SetUp() override
    {
        protocol.reset(new wolkabout::JsonStatusProtocol);
        gatewayProtocol.reset(new wolkabout::JsonGatewayStatusProtocol);
        deviceRepository = std::unique_ptr<MockRepository>(new MockRepository());
        connectionStatusListener = std::make_shared<MockConnectionStatusListener>();
        platformOutboundMessageHandler =
          std::unique_ptr<PlatformOutboundMessageHandler>(new PlatformOutboundMessageHandler());
        deviceOutboundMessageHandler =
          std::unique_ptr<DeviceOutboundMessageHandler>(new DeviceOutboundMessageHandler());
        deviceStatusService = std::unique_ptr<wolkabout::DeviceStatusService>(new wolkabout::DeviceStatusService(
          GATEWAY_KEY, *protocol, *gatewayProtocol, deviceRepository.get(), *platformOutboundMessageHandler,
          *deviceOutboundMessageHandler, std::chrono::seconds{60}));
    }

    void TearDown() override { remove(DEVICE_REPOSITORY_PATH); }

    std::unique_ptr<wolkabout::StatusProtocol> protocol;
    std::unique_ptr<wolkabout::GatewayStatusProtocol> gatewayProtocol;
    std::unique_ptr<MockRepository> deviceRepository;
    std::unique_ptr<PlatformOutboundMessageHandler> platformOutboundMessageHandler;
    std::unique_ptr<DeviceOutboundMessageHandler> deviceOutboundMessageHandler;
    std::shared_ptr<MockConnectionStatusListener> connectionStatusListener;
    std::unique_ptr<wolkabout::DeviceStatusService> deviceStatusService;

    static constexpr const char* DEVICE_REPOSITORY_PATH = "testsDeviceRepository.db";
    static constexpr const char* GATEWAY_KEY = "GATEWAY_KEY";
};
}    // namespace

TEST_F(DeviceStatusService, Given_When_MessageFromPlatformWithInvalidChannelDirectionIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY");
    deviceStatusService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DeviceStatusService, Given_When_MessageFromPlatformWithInvalidMessageTypeIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/status_get/g/GATEWAY_KEY/d/DEVICE_KEY");
    deviceStatusService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DeviceStatusService, Given_When_MessageFromPlatformWithInvalidDeviceTypeIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/status/d/GATEWAY_KEY/d/DEVICE_KEY");
    deviceStatusService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DeviceStatusService, Given_When_MessageFromPlatformIsReceived_Then_MessageIsSentToDevice)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/subdevice_status_request/g/GATEWAY_KEY/d/DEVICE_KEY");
    deviceStatusService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().front()->getChannel(),
              "p2d/subdevice_status_request/d/DEVICE_KEY");
}

TEST_F(DeviceStatusService, Given_When_MessageFromDeviceWithInvalidChannelDirectionIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/status/d/DEVICE_KEY");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DeviceStatusService, Given_When_MessageFromDeviceWithInvalidMessageTypeIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/status_reponse/d/DEVICE_KEY");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DeviceStatusService,
       Given_When_LastWillWithKeyMessageFromDeviceIsReceived_Then_DeviceStatusMessageIsSentToPlatform)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "lastwill/DEVICE_KEY");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().front()->getChannel(),
              "d2p/subdevice_status_update/g/GATEWAY_KEY/d/DEVICE_KEY");
}

TEST_F(DeviceStatusService,
       Given_When_LastWillWithoutKeyEmptyPayloadMessageFromDeviceIsReceived_Then_NoMessageIsSentToPlatform)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "lastwill");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(DeviceStatusService,
       Given_When_LastWillWithoutKeyInvalidPayloadMessageFromDeviceIsReceived_Then_NoMessageIsSentToPlatform)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("'}[Invalid key list]", "lastwill");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(
  DeviceStatusService,
  Given_When_LastWillWithoutKeySingleKeyPayloadMessageFromDeviceIsReceived_Then_DeviceStatusMessageIsSentToPlatform)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("[\"DEVICE_KEY\"]", "lastwill");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().front()->getChannel(),
              "d2p/subdevice_status_update/g/GATEWAY_KEY/d/DEVICE_KEY");
}

TEST_F(
  DeviceStatusService,
  Given_When_LastWillWithoutKeyMultipleKeysPayloadMessageFromDeviceIsReceived_Then_DeviceStatusMessageIsSentToPlatform)
{
    // Given
    // Intentionally left empty

    // When
    auto message =
      std::make_shared<wolkabout::Message>("[\"DEVICE_KEY_1\", \"DEVICE_KEY_2\", \"DEVICE_KEY_3\"]", "lastwill");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 3);
    ASSERT_EQ(platformOutboundMessageHandler->getMessages()[0]->getChannel(),
              "d2p/subdevice_status_update/g/GATEWAY_KEY/d/DEVICE_KEY_1");
    ASSERT_EQ(platformOutboundMessageHandler->getMessages()[1]->getChannel(),
              "d2p/subdevice_status_update/g/GATEWAY_KEY/d/DEVICE_KEY_2");
    ASSERT_EQ(platformOutboundMessageHandler->getMessages()[2]->getChannel(),
              "d2p/subdevice_status_update/g/GATEWAY_KEY/d/DEVICE_KEY_3");
}

TEST_F(DeviceStatusService, Given_When_StatusMessageFromDeviceWithInvalidDeviceTypeIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/status/p/DEVICE_KEY");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DeviceStatusService, Given_When_StatusMessageFromDeviceIsReceived_Then_MessageIsSentToPlatform)
{
    // Given
    // Intentionally left empty

    // When
    auto message =
      std::make_shared<wolkabout::Message>("{\"state\":\"CONNECTED\"}", "d2p/subdevice_status_update/d/DEVICE_KEY");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().front()->getChannel(),
              "d2p/subdevice_status_update/g/GATEWAY_KEY/d/DEVICE_KEY");
}

TEST_F(DeviceStatusService, GivenGatewayInRepository_When_ConnectedToDevices_Then_StatusRequestNotSent)
{
    // Given
    std::vector<std::string> keys = {GATEWAY_KEY};

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::ReturnNew<std::vector<std::string>>(keys));

    // When
    deviceStatusService->connected();

    // Then
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().size(), 0);
}

TEST_F(DeviceStatusService,
       GivenGatewayAndOneDeviceInRepository_When_ConnectedToDevices_Then_StatusRequestIsSentToDevice)
{
    // Given
    const std::string deviceKey1 = "KEY1";

    std::vector<std::string> keys = {GATEWAY_KEY, deviceKey1};

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::ReturnNew<std::vector<std::string>>(keys));

    // When
    deviceStatusService->connected();

    // Then
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().size(), 1);
}

TEST_F(DeviceStatusService,
       GivenGatewayAndMultipleDevicesInRepository_When_ConnectedToDevices_Then_StatusRequestIsSentToEachDevice)
{
    // Given
    const std::string deviceKey1 = "KEY1";
    const std::string deviceKey2 = "KEY2";
    const std::string deviceKey3 = "KEY3";

    std::vector<std::string> keys = {GATEWAY_KEY, deviceKey1, deviceKey2, deviceKey3};

    ON_CALL(*deviceRepository, findAllDeviceKeysProxy())
      .WillByDefault(testing::ReturnNew<std::vector<std::string>>(keys));

    // When
    deviceStatusService->connected();

    // Then
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().size(), 3);
}
