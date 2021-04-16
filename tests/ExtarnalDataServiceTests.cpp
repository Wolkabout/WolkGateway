#include "MockRepository.h"
#include "OutboundMessageHandler.h"
#include "core/model/Message.h"
#include "core/protocol/json/JsonProtocol.h"
#include "protocol/json/JsonGatewayDataProtocol.h"
#include "repository/SQLiteDeviceRepository.h"
#include "service/data/DataService.h"
#include "service/data/ExternalDataService.h"
#include "service/data/InternalDataService.h"

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

class ExternalDataService : public ::testing::Test
{
public:
    void SetUp() override
    {
        protocol.reset(new wolkabout::JsonProtocol(true));
        gateawayProtocol = std::unique_ptr<wolkabout::GatewayDataProtocol>(new wolkabout::JsonGatewayDataProtocol());

        deviceRepository = std::unique_ptr<MockRepository>(new MockRepository());
        platformOutboundMessageHandler =
          std::unique_ptr<PlatformOutboundMessageHandler>(new PlatformOutboundMessageHandler());
        dataService = std::unique_ptr<wolkabout::ExternalDataService>(new wolkabout::ExternalDataService(
          GATEWAY_KEY, *protocol, *gateawayProtocol, *platformOutboundMessageHandler, nullptr));
    }

    void TearDown() override { remove(DEVICE_REPOSITORY_PATH); }

    std::unique_ptr<wolkabout::DataProtocol> protocol;
    std::unique_ptr<wolkabout::GatewayDataProtocol> gateawayProtocol;
    std::unique_ptr<MockRepository> deviceRepository;
    std::unique_ptr<PlatformOutboundMessageHandler> platformOutboundMessageHandler;
    std::unique_ptr<wolkabout::ExternalDataService> dataService;

    static constexpr const char* DEVICE_REPOSITORY_PATH = "testsDeviceRepository.db";
    static constexpr const char* GATEWAY_KEY = "GATEWAY_KEY";
};
}    // namespace

TEST_F(ExternalDataService, Given_When_MessageFromPlatformWithInvalidChannelDirectionIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "d2p/actuator_set/g/GATEWAY_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(ExternalDataService, Given_When_MessageFromPlatformWithMissingDeviceTypeIsReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/actuator_set/GATEWAY_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(ExternalDataService, DISABLED_Given_When_MessageFromPlatformForDeviceIsReceived_Then_MessageIsSentToDeviceModule)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/actuator_set/g/GATEWAY_KEY/d/DEVICE_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(ExternalDataService,
       Given_When_MessageFromPlatformForDaviceWithInvalidDeviceTypeIsReceived_Then_MessageSentIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/actuator_set/g/DEVICE_KEY/r/REF");
    dataService->platformMessageReceived(message);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(ExternalDataService, Given_When_EmptyReadingsAreReceived_Then_MessageIsIgnored)
{
    // Given
    // Intentionally left empty

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/sensor_reading/g/GATEWAY_KEY/r/REF");
    dataService->addSensorReadings("DEVICE_KEY", {});

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(ExternalDataService, Given_When_MessageFromDeviceIsReceived_Then_MessageIsSentToPlatform)
{
    // When
    dataService->addSensorReading("DEVICE_KEY", {"5", "REF"});

    // Then
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 1);
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().front()->getChannel(),
              "d2p/sensor_reading/g/GATEWAY_KEY/d/DEVICE_KEY/r/REF");
}
