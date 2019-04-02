#include "MockRepository.h"
#include "OutboundMessageHandler.h"
#include "model/Message.h"
#include "protocol/json/JsonGatewayDataProtocol.h"
#include "protocol/json/JsonProtocol.h"
#include "repository/SQLiteDeviceRepository.h"
#include "service/DataService.h"

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
        protocol.reset(new wolkabout::JsonProtocol(true));
        gateawayProtocol = std::unique_ptr<wolkabout::GatewayDataProtocol>(new wolkabout::JsonGatewayDataProtocol());

        deviceRepository = std::unique_ptr<MockRepository>(new MockRepository());
        platformOutboundMessageHandler =
          std::unique_ptr<PlatformOutboundMessageHandler>(new PlatformOutboundMessageHandler());
        deviceOutboundMessageHandler =
          std::unique_ptr<DeviceOutboundMessageHandler>(new DeviceOutboundMessageHandler());
        dataService = std::unique_ptr<wolkabout::DataService>(
          new wolkabout::DataService(GATEWAY_KEY, *protocol, *gateawayProtocol, deviceRepository.get(),
                                     *platformOutboundMessageHandler, *deviceOutboundMessageHandler, nullptr));
    }

    void TearDown() override { remove(DEVICE_REPOSITORY_PATH); }

    std::unique_ptr<wolkabout::DataProtocol> protocol;
    std::unique_ptr<wolkabout::GatewayDataProtocol> gateawayProtocol;
    std::unique_ptr<MockRepository> deviceRepository;
    std::unique_ptr<PlatformOutboundMessageHandler> platformOutboundMessageHandler;
    std::unique_ptr<DeviceOutboundMessageHandler> deviceOutboundMessageHandler;
    std::unique_ptr<wolkabout::DataService> dataService;

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
    auto message = std::make_shared<wolkabout::Message>("", "p2d/sensor_reading/g/GATEWAY_KEY/r/REF");
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
    auto message = std::make_shared<wolkabout::Message>("", "d2p/sensor_reading/GATEWAY_KEY/r/REF");
    dataService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DataService, Given_When_MessageFromDeviceWithIncorrectDeviceTypeIsReceived_Then_MessageIsIgnored)
{
    // Given
    ON_CALL(*deviceRepository, findByDeviceKeyProxy("GATEWAY_KEY"))
      .WillByDefault(testing::Return(new wolkabout::DetailedDevice(
        "", "GATEWAY_KEY",
        wolkabout::DeviceTemplate{{},
                                  {wolkabout::SensorTemplate{"", "REF", wolkabout::DataType::NUMERIC, "", {0}, {100}}},
                                  {},
                                  {},
                                  "",
                                  {},
                                  {},
                                  {}})));

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/sensor_reading/k/GATEWAY_KEY/r/REF");
    dataService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
}

TEST_F(DataService, Given_When_MessageFromDeviceIsReceived_Then_MessageIsSentToPlatform)
{
    // Given
    ON_CALL(*deviceRepository, findByDeviceKeyProxy("DEVICE_KEY"))
      .WillByDefault(testing::Return(new wolkabout::DetailedDevice(
        "", "DEVICE_KEY",
        wolkabout::DeviceTemplate{{},
                                  {wolkabout::SensorTemplate{"", "REF", wolkabout::DataType::NUMERIC, "", {0}, {100}}},
                                  {},
                                  {},
                                  "",
                                  {},
                                  {},
                                  {}})));

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/sensor_reading/d/DEVICE_KEY/r/REF");
    dataService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().front()->getChannel(),
              "d2p/sensor_reading/g/GATEWAY_KEY/d/DEVICE_KEY/r/REF");
}

TEST_F(DataService,
       Given_MessageThatIsNotInLineWithDeviceTemplate_When_MessageIsReceived_Then_MessageIsNotSentToPlatform)
{
    // Given
    auto message = std::make_shared<wolkabout::Message>("", "d2p/sensor_reading/d/DEVICE_KEY/r/REF");

    ON_CALL(*deviceRepository, findByDeviceKeyProxy("DEVICE_KEY"))
      .WillByDefault(testing::Return(new wolkabout::DetailedDevice(
        "", "DEVICE_KEY",
        wolkabout::DeviceTemplate{{},
                                  {wolkabout::SensorTemplate{"", "ref", wolkabout::DataType::NUMERIC, "", {0}, {100}}},
                                  {},
                                  {},
                                  "",
                                  {},
                                  {},
                                  {}})));

    // When
    dataService->deviceMessageReceived(message);

    // Then
    ASSERT_TRUE(deviceOutboundMessageHandler->getMessages().empty());
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}
