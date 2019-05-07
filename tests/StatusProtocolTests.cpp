#include "model/DeviceStatus.h"
#include "model/Message.h"
#include "protocol/json/JsonGatewayStatusProtocol.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace
{
class JsonGatewayStatusProtocol : public ::testing::Test
{
public:
    void SetUp() override
    {
        protocol = std::unique_ptr<wolkabout::JsonGatewayStatusProtocol>(new wolkabout::JsonGatewayStatusProtocol());
    }

    void TearDown() override {}

    std::unique_ptr<wolkabout::JsonGatewayStatusProtocol> protocol;
};
}    // namespace

TEST_F(JsonGatewayStatusProtocol,
       Given_StatusChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string statusChannel = "d2p/status/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(statusChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(JsonGatewayStatusProtocol,
       Given_LastWillChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string lastwillChannel = "lastwill/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(lastwillChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(JsonGatewayStatusProtocol,
       Given_LastWillChannelForDeviceNoKey_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToEmpty)
{
    // Given
    const std::string lastwillChannel = "lastwill";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(lastwillChannel);

    // Then
    ASSERT_EQ("", deviceKey);
}

TEST_F(
  JsonGatewayStatusProtocol,
  Given_LastWillChannelAndPayloadForDevice_When_DeviceKeysAreExtracted_Then_ExtractedDeviceKeysAreEqualToDeviceKeys)
{
    // Given
    const std::string lastwillChannel = "lastwill";
    const std::string lastwillPayload = "[\"DEVICE_KEY_1\", \"KEY_OF_DEVICE_2\", \"testKey\"]";

    // When
    const auto deviceKeys = protocol->extractDeviceKeysFromContent(lastwillPayload);

    // Then
    ASSERT_EQ(deviceKeys.size(), 3);

    auto it = std::find(deviceKeys.begin(), deviceKeys.end(), "DEVICE_KEY_1");
    ASSERT_TRUE(it != deviceKeys.end());

    auto it2 = std::find(deviceKeys.begin(), deviceKeys.end(), "KEY_OF_DEVICE_2");
    ASSERT_TRUE(it2 != deviceKeys.end());

    auto it3 = std::find(deviceKeys.begin(), deviceKeys.end(), "testKey");
    ASSERT_TRUE(it3 != deviceKeys.end());
}

TEST_F(JsonGatewayStatusProtocol, Given_Channels_When_DeviceChannelsAreRequested_Then_DeviceChannelsMatchChannels)
{
    // Given
    const std::vector<std::string> channels{"d2p/subdevice_status_response/d/#", "d2p/subdevice_status_update/d/#",
                                            "lastwill/#"};

    // When
    const auto deviceChannels = protocol->getInboundChannels();

    // Then
    for (const auto& channel : channels)
    {
        auto it = std::find(deviceChannels.begin(), deviceChannels.end(), channel);
        ASSERT_TRUE(it != deviceChannels.end());
    }

    ASSERT_EQ(deviceChannels.size(), channels.size());
}

TEST_F(JsonGatewayStatusProtocol,
       Given_StatusResponseMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsStatusResponse)
{
    // Given
    const std::string statusResponseChannel = "d2p/subdevice_status_response/d/DEVICE_KEY";
    const auto message = std::make_shared<wolkabout::Message>("", statusResponseChannel);

    // When
    const bool isStatusResponse = protocol->isStatusResponseMessage(*message);

    // Then
    ASSERT_TRUE(isStatusResponse);
}

TEST_F(JsonGatewayStatusProtocol, Given_LastWillMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsLastWill)
{
    // Given
    const std::string lastWillChannel = "lastwill/DEVICE_KEY";
    const auto message = std::make_shared<wolkabout::Message>("", lastWillChannel);

    // When
    const bool isLastwill = protocol->isLastWillMessage(*message);

    // Then
    ASSERT_TRUE(isLastwill);
}

TEST_F(JsonGatewayStatusProtocol, Given_DeviceStatusRequest_When_MessageIsCreated_Then_MessageChannelMatchKeys)
{
    // Given

    // When
    const auto message = protocol->makeDeviceStatusRequestMessage("DEVICE_KEY");

    // Then
    ASSERT_TRUE(message != nullptr);
    ASSERT_EQ(message->getChannel(), "p2d/subdevice_status_request/d/DEVICE_KEY");
}

TEST_F(JsonGatewayStatusProtocol, Given_StatusResponseMessage_When_StatusResponseIsCreated_Then_StatusMatchesPayload)
{
    // Given
    const std::string jsonPayload = "{\"state\":\"CONNECTED\"}";
    const std::string channel = "d2p/subdevice_status_response/d/DEVICE_KEY";
    const auto message = std::make_shared<wolkabout::Message>(jsonPayload, channel);

    // When
    const auto response = protocol->makeDeviceStatusResponse(*message);

    // Then
    ASSERT_TRUE(response);
    ASSERT_EQ(response->getStatus(), wolkabout::DeviceStatus::Status::CONNECTED);
}
