#include "ConnectionStatusListener.h"
#include "MockConnectionStatusListener.h"
#include "MockRepository.h"
#include "OutboundMessageHandler.h"
#include "model/Message.h"
#include "protocol/json/JsonGatewayStatusProtocol.h"
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
        protocol = std::unique_ptr<wolkabout::GatewayStatusProtocol>(new wolkabout::JsonGatewayStatusProtocol);
        deviceRepository = std::unique_ptr<MockRepository>(new MockRepository());
        connectionStatusListener = std::make_shared<MockConnectionStatusListener>();
        platformOutboundMessageHandler =
          std::unique_ptr<PlatformOutboundMessageHandler>(new PlatformOutboundMessageHandler());
        deviceOutboundMessageHandler =
          std::unique_ptr<DeviceOutboundMessageHandler>(new DeviceOutboundMessageHandler());
        deviceStatusService = std::unique_ptr<wolkabout::DeviceStatusService>(
          new wolkabout::DeviceStatusService(GATEWAY_KEY, *protocol, *deviceRepository, *platformOutboundMessageHandler,
                                             *deviceOutboundMessageHandler, std::chrono::seconds{60}));
        deviceStatusService->setGatewayModuleConnectionStatusListener(connectionStatusListener);
    }

    void TearDown() override { remove(DEVICE_REPOSITORY_PATH); }

    std::unique_ptr<wolkabout::GatewayStatusProtocol> protocol;
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

// Disabled as platform OK response status is not routed to device
TEST_F(DeviceStatusService, DISABLED_Given_When_MessageFromPlatformIsReceived_Then_MessageIsSentToDevice)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/status/g/GATEWAY_KEY/d/DEVICE_KEY");
    deviceStatusService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().front()->getChannel(), "p2d/status/d/DEVICE_KEY");
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
       Given_When_LastWillWithKeyMessageFromGatewayIsReceived_Then_GatewayDisconnectedCallbackIsCalled)
{
    // Given
    EXPECT_CALL(*connectionStatusListener, disconnected()).Times(1);

    // When
    auto message = std::make_shared<wolkabout::Message>("", "lastwill/GATEWAY_KEY");
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
              "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY");
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
              "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY");
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
              "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY_1");
    ASSERT_EQ(platformOutboundMessageHandler->getMessages()[1]->getChannel(),
              "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY_2");
    ASSERT_EQ(platformOutboundMessageHandler->getMessages()[2]->getChannel(),
              "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY_3");
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

TEST_F(DeviceStatusService, Given_When_StatusMessageFromGatewayWithInvalidStatusValueIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    const std::string jsonPayload = "{\"state\":\"CONNECTEDD\"}";
    auto message = std::make_shared<wolkabout::Message>(jsonPayload, "d2p/status/d/GATEWAY_KEY");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DeviceStatusService,
       Given_When_StatusMessageFromGatewayConnectedStatusValueIsReceived_Then_GatewayConnectedCallbackIsCalled)
{
    // Given
    EXPECT_CALL(*connectionStatusListener, connected()).Times(1);

    // When
    const std::string jsonPayload = "{\"state\":\"CONNECTED\"}";
    auto message = std::make_shared<wolkabout::Message>(jsonPayload, "d2p/status/d/GATEWAY_KEY");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DeviceStatusService,
       Given_When_StatusMessageFromGatewaySleepStatusValueIsReceived_Then_GatewayDisconnectedCallbackIsCalled)
{
    // Given
    EXPECT_CALL(*connectionStatusListener, disconnected()).Times(1);

    // When
    const std::string jsonPayload = "{\"state\":\"SLEEP\"}";
    auto message = std::make_shared<wolkabout::Message>(jsonPayload, "d2p/status/d/GATEWAY_KEY");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DeviceStatusService,
       Given_When_StatusMessageFromGatewayOfflineStatusValueIsReceived_Then_GatewayDisconnectedCallbackIsCalled)
{
    // Given
    EXPECT_CALL(*connectionStatusListener, disconnected()).Times(1);

    // When
    const std::string jsonPayload = "{\"state\":\"OFFLINE\"}";
    auto message = std::make_shared<wolkabout::Message>(jsonPayload, "d2p/status/d/GATEWAY_KEY");
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
    auto message = std::make_shared<wolkabout::Message>("{\"state\":\"CONNECTED\"}", "d2p/status/d/DEVICE_KEY");
    deviceStatusService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().front()->getChannel(),
              "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY");
}
