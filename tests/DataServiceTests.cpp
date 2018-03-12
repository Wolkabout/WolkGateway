#include "MockRepository.h"
#include "OutboundMessageHandler.h"
#include "connectivity/json/JsonProtocol.h"
#include "model/Message.h"
#include "repository/SQLiteDeviceRepository.h"

#define private public
#define protected public
#include "service/DataService.h"
#undef private
#undef protected

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

class DataService : public ::testing::Test
{
public:
    void SetUp() override
    {
        deviceRepository = std::unique_ptr<MockRepository>(new MockRepository());
        platformOutboundMessageHandler =
          std::unique_ptr<PlatformOutboundMessageHandler>(new PlatformOutboundMessageHandler());
        deviceOutboundMessageHandler =
          std::unique_ptr<DeviceOutboundMessageHandler>(new DeviceOutboundMessageHandler());
        dataService = std::unique_ptr<wolkabout::DataService<wolkabout::JsonProtocol>>(
          new wolkabout::DataService<wolkabout::JsonProtocol>(
            GATEWAY_KEY, *deviceRepository, *platformOutboundMessageHandler, *deviceOutboundMessageHandler));
    }

    void TearDown() override { remove(DEVICE_REPOSITORY_PATH); }

    std::unique_ptr<MockRepository> deviceRepository;
    std::unique_ptr<PlatformOutboundMessageHandler> platformOutboundMessageHandler;
    std::unique_ptr<DeviceOutboundMessageHandler> deviceOutboundMessageHandler;
    std::unique_ptr<wolkabout::DataService<wolkabout::JsonProtocol>> dataService;

    static constexpr const char* DEVICE_REPOSITORY_PATH = "testsDeviceRepository.db";
    static constexpr const char* GATEWAY_KEY = "GATEWAY_KEY";
};
}    // namespace

TEST_F(DataService, Given_When_MessageFromPlatformWithInvalidChannelDirectionIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/actuator_set/g/GATEWAY_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DataService, Given_When_MessageFromPlatformWithMissingDeviceTypeIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/actuator_set/GATEWAY_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DataService,
       Given_GatewayModuleIsConnected_When_MessageFromPlatformForGatewayIsReceived_Then_MessageIsSentToGatewayModule)
{
    // Given
    dataService->m_gatewayModuleConnected = true;

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/actuator_set/g/GATEWAY_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().front()->getChannel(),
              "p2d/actuator_set/d/GATEWAY_KEY/r/REF");
}

TEST_F(
  DataService,
  Given_GatewayModuleIsConnected_When_MessageFromPlatformForGatewayWithInvalidDeviceTypeIsReceived_Then_MessageIsIgnored)
{
    // Given
    dataService->m_gatewayModuleConnected = true;

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/actuator_set/d/GATEWAY_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DataService,
       Given_GatewayModuleIsDisonnected_When_MessageFromPlatformForGatewayIsReceived_Then_MessageSentToGatewayModule)
{
    // Given
    dataService->m_gatewayModuleConnected = false;

    ON_CALL(*deviceRepository, findByDeviceKeyProxy("GATEWAY_KEY"))
      .WillByDefault(testing::Return(new wolkabout::Device(
        "", "GATEWAY_KEY",
        wolkabout::DeviceManifest{
          "",
          "",
          "",
          "",
          {},
          {},
          {},
          {wolkabout::ActuatorManifest{"", "REF", "", "", "", wolkabout::ActuatorManifest::DataType::NUMERIC, 1, 0,
                                       100}}})));

    // When
    auto message = std::make_shared<wolkabout::Message>("{\"value\":\"15\"}", "p2d/actuator_set/g/GATEWAY_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().front()->getChannel(),
              "d2p/actuator_status/g/GATEWAY_KEY/r/REF");
}

TEST_F(
  DataService,
  Given_GatewayModuleIsDisonnected_When_MessageFromPlatformForGatewayWithMissingReferenceIsReceived_Then_MessageIsIgnored)
{
    // Given
    dataService->m_gatewayModuleConnected = false;

    ON_CALL(*deviceRepository, findByDeviceKeyProxy("GATEWAY_KEY"))
      .WillByDefault(testing::Return(new wolkabout::Device(
        "", "GATEWAY_KEY",
        wolkabout::DeviceManifest{
          "",
          "",
          "",
          "",
          {},
          {},
          {},
          {wolkabout::ActuatorManifest{"", "OTHER_REF", "", "", "", wolkabout::ActuatorManifest::DataType::NUMERIC, 1,
                                       0, 100}}})));

    // When
    auto message = std::make_shared<wolkabout::Message>("{\"value\":\"15\"}", "p2d/actuator_set/g/GATEWAY_KEY/r/");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(
  DataService,
  Given_GatewayModuleIsDisonnected_When_MessageFromPlatformForGatewayWithUndefinedReferenceIsReceived_Then_MessageIsIgnored)
{
    // Given
    dataService->m_gatewayModuleConnected = false;

    ON_CALL(*deviceRepository, findByDeviceKeyProxy("GATEWAY_KEY"))
      .WillByDefault(testing::Return(new wolkabout::Device(
        "", "GATEWAY_KEY",
        wolkabout::DeviceManifest{
          "",
          "",
          "",
          "",
          {},
          {},
          {},
          {wolkabout::ActuatorManifest{"", "OTHER_REF", "", "", "", wolkabout::ActuatorManifest::DataType::NUMERIC, 1,
                                       0, 100}}})));

    // When
    auto message = std::make_shared<wolkabout::Message>("{\"value\":\"15\"}", "p2d/actuator_set/g/GATEWAY_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(
  DataService,
  Given_GatewayModuleIsDisonnectedAndGatewayDeviceIsNotInRepository_When_MessageFromPlatformForGatewayIsReceived_Then_MessageIsIgnored)
{
    // Given
    dataService->m_gatewayModuleConnected = false;

    ON_CALL(*deviceRepository, findByDeviceKeyProxy("GATEWAY_KEY")).WillByDefault(testing::Return(nullptr));

    // When
    auto message = std::make_shared<wolkabout::Message>("{\"value\":\"15\"}", "p2d/actuator_set/g/GATEWAY_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(DataService, Given_When_MessageFromPlatformForDeviceIsReceived_Then_MessageIsSentToDeviceModule)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/actuator_set/g/GATEWAY_KEY/d/DEVICE_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(deviceOutboundMessageHandler->getMessages().front()->getChannel(), "p2d/actuator_set/d/DEVICE_KEY/r/REF");
}

TEST_F(DataService, Given_When_MessageFromPlatformForDaviceWithInvalidDeviceTypeIsReceived_Then_MessageSentIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/actuator_set/g/DEVICE_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DataService, Given_When_MessageFromDeviceWithInvalidChannelDirectionIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/sensor_readings/g/GATEWAY_KEY/r/REF");
    dataService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DataService, Given_When_MessageFromDeviceWithMissingDeviceTypeIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/sensor_readings/GATEWAY_KEY/r/REF");
    dataService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(
  DataService,
  Given_GatewayModuleIsDiconnected_When_MessageFromGatewayWithIncorrectDeviceTypeIsReceived_Then_MessageIsIgnoredAndGatewayIsConnected)
{
    // Given
    dataService->m_gatewayModuleConnected = false;

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/sensor_readings/g/GATEWAY_KEY/r/REF");
    dataService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(dataService->m_gatewayModuleConnected);
}

TEST_F(
  DataService,
  Given_GatewayModuleIsDiconnected_When_MessageFromGatewayIsReceived_Then_MessageIsSentToPlatformAndGatewayIsConnected)
{
    // Given
    dataService->m_gatewayModuleConnected = false;

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/sensor_readings/d/GATEWAY_KEY/r/REF");
    dataService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().front()->getChannel(),
              "d2p/sensor_readings/g/GATEWAY_KEY/r/REF");
    ASSERT_TRUE(dataService->m_gatewayModuleConnected);
}

TEST_F(DataService, Given_When_MessageFromDeviceWithIncorrectDeviceTypeIsReceived_Then_MessageIsIgnored)
{
    // Given

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/sensor_readings/k/GATEWAY_KEY/r/REF");
    dataService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DataService, Given_When_MessageFromDeviceIsReceived_Then_MessageIsSentToPlatform)
{
    // Given

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/sensor_readings/d/DEVICE_KEY/r/REF");
    dataService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().front()->getChannel(),
              "d2p/sensor_readings/g/GATEWAY_KEY/d/DEVICE_KEY/r/REF");
}
