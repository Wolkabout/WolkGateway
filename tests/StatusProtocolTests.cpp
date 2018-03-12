#include "connectivity/json/StatusProtocol.h"
#include "model/DeviceStatusResponse.h"
#include "model/Message.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(StatusProtocol, Given_Name_When_ProtocolNameIsRequested_Then_NameIsEqualToProtocolName)
{
    // Given
    const std::string name = "StatusProtocol";

    // When
    const std::string protocolName = wolkabout::StatusProtocol::getName();

    // Then
    ASSERT_EQ(name, protocolName);
}

TEST(StatusProtocol, Given_StatusChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string statusChannel = "d2p/status/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::StatusProtocol::extractDeviceKeyFromChannel(statusChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(StatusProtocol, Given_LastWillChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string lastwillChannel = "lastwill/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::StatusProtocol::extractDeviceKeyFromChannel(lastwillChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(StatusProtocol,
     Given_LastWillChannelForDeviceNoKey_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToEmpty)
{
    // Given
    const std::string lastwillChannel = "lastwill";

    // When
    const std::string deviceKey = wolkabout::StatusProtocol::extractDeviceKeyFromChannel(lastwillChannel);

    // Then
    ASSERT_EQ("", deviceKey);
}

TEST(StatusProtocol,
     Given_LastWillChannelAndPayloadForDevice_When_DeviceKeysAreExtracted_Then_ExtractedDeviceKeysAreEqualToDeviceKeys)
{
    // Given
    const std::string lastwillChannel = "lastwill";
    const std::string lastwillPayload = "[\"DEVICE_KEY_1\", \"KEY_OF_DEVICE_2\", \"testKey\"]";

    // When
    const auto deviceKeys = wolkabout::StatusProtocol::deviceKeysFromContent(lastwillPayload);

    // Then
    ASSERT_EQ(deviceKeys.size(), 3);

    auto it = std::find(deviceKeys.begin(), deviceKeys.end(), "DEVICE_KEY_1");
    ASSERT_TRUE(it != deviceKeys.end());

    auto it2 = std::find(deviceKeys.begin(), deviceKeys.end(), "KEY_OF_DEVICE_2");
    ASSERT_TRUE(it2 != deviceKeys.end());

    auto it3 = std::find(deviceKeys.begin(), deviceKeys.end(), "testKey");
    ASSERT_TRUE(it3 != deviceKeys.end());
}

TEST(StatusProtocol, Given_GatewayDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToDeviceChannel)
{
    // Given
    const std::string channel = "p2d/status/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const std::string routedChannel = wolkabout::StatusProtocol::routePlatformMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("p2d/status/d/DEVICE_KEY", routedChannel);
}

TEST(StatusProtocol, Given_InvalidGatewayDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToEmpty)
{
    // Given
    const std::string channel = "p2d/status/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const std::string routedChannel = wolkabout::StatusProtocol::routePlatformMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("", routedChannel);
}

TEST(StatusProtocol, Given_DeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToGatewayDeviceChannel)
{
    // Given
    const std::string channel = "d2p/status/d/DEVICE_KEY";

    // When
    const std::string routedChannel = wolkabout::StatusProtocol::routeDeviceMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY", routedChannel);
}

TEST(StatusProtocol, Given_InvalidDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToEmpty)
{
    // Given
    const std::string channel = "d2p/status/DEVICE_KEY";

    // When
    const std::string routedChannel = wolkabout::StatusProtocol::routeDeviceMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("", routedChannel);
}

TEST(StatusProtocol, Given_MessageFromPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsFromPlatform)
{
    // Given
    const std::string statusChannel = "p2d/status/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isMessageFromPlatform = wolkabout::StatusProtocol::isMessageFromPlatform(statusChannel);

    // Then
    ASSERT_TRUE(isMessageFromPlatform);
}

TEST(StatusProtocol,
     Given_MessageFromDevice_When_MessageDirectionIsChecked_Then_MessageDirectionDoesNotEqualFromPlatform)
{
    // Given
    const std::string statusChannel = "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isMessageFromPlatform = wolkabout::StatusProtocol::isMessageFromPlatform(statusChannel);

    // Then
    ASSERT_FALSE(isMessageFromPlatform);
}

TEST(StatusProtocol, Given_MessageToPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsToPlatform)
{
    // Given
    const std::string statusChannel = "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isMessageToPlatform = wolkabout::StatusProtocol::isMessageToPlatform(statusChannel);

    // Then
    ASSERT_TRUE(isMessageToPlatform);
}

TEST(StatusProtocol, Given_MessageToDevice_When_MessageDirectionIsChecked_Then_MessageDirectionDoesNotEqualToPlatform)
{
    // Given
    const std::string statusChannel = "p2d/status/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isMessageToPlatform = wolkabout::StatusProtocol::isMessageToPlatform(statusChannel);

    // Then
    ASSERT_FALSE(isMessageToPlatform);
}

TEST(StatusProtocol, Given_Channels_When_DeviceChannelsAreRequested_Then_DeviceChannelsMatchChannels)
{
    // Given
    const std::vector<std::string> channels{"d2p/status/#", "lastwill/#"};

    // When
    const auto deviceChannels = wolkabout::StatusProtocol::getDeviceChannels();

    // Then
    for (const auto& channel : channels)
    {
        auto it = std::find(deviceChannels.begin(), deviceChannels.end(), channel);
        ASSERT_TRUE(it != deviceChannels.end());
    }
}

TEST(StatusProtocol, Given_Channels_When_PlatformChannelsAreRequested_Then_PlatformChannelsMatchChannels)
{
    // Given
    const std::vector<std::string> channels{"p2d/status/#"};

    // When
    const auto platformChannels = wolkabout::StatusProtocol::getPlatformChannels();

    // Then
    for (const auto& channel : channels)
    {
        auto it = std::find(platformChannels.begin(), platformChannels.end(), channel);
        ASSERT_TRUE(it != platformChannels.end());
    }
}

TEST(StatusProtocol, Given_StatusRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsStatusRequest)
{
    // Given
    const std::string statusRequestChannel = "p2d/status/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isStatusRequest = wolkabout::StatusProtocol::isStatusRequestMessage(statusRequestChannel);

    // Then
    ASSERT_TRUE(isStatusRequest);
}

TEST(StatusProtocol, Given_StatusResponseMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsStatusResponse)
{
    // Given
    const std::string statusResponseChannel = "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isStatusResponse = wolkabout::StatusProtocol::isStatusResponseMessage(statusResponseChannel);

    // Then
    ASSERT_TRUE(isStatusResponse);
}

TEST(StatusProtocol, Given_LastWillMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsLastWill)
{
    // Given
    const std::string lastWillChannel = "lastwill/DEVICE_KEY";

    // When
    const bool isLastwill = wolkabout::StatusProtocol::isLastWillMessage(lastWillChannel);

    // Then
    ASSERT_TRUE(isLastwill);
}

TEST(StatusProtocol, Given_DeviceStatusResponse_When_MessageIsCreated_Then_MessageChannelMatchKeys)
{
    // Given

    // When
    const auto message = wolkabout::StatusProtocol::messageFromDeviceStatusResponse(
      "GATEWAY_KEY", "DEVICE_KEY", wolkabout::DeviceStatusResponse::Status::CONNECTED);

    // Then
    ASSERT_TRUE(message != nullptr);
    ASSERT_EQ(message->getChannel(), "d2p/status/g/GATEWAY_KEY/d/DEVICE_KEY");
}

TEST(StatusProtocol, Given_DeviceStatusRequest_When_MessageIsCreated_Then_MessageChannelMatchKeys)
{
    // Given

    // When
    const auto message = wolkabout::StatusProtocol::messageFromDeviceStatusRequest("DEVICE_KEY");

    // Then
    ASSERT_TRUE(message != nullptr);
    ASSERT_EQ(message->getChannel(), "p2d/status/d/DEVICE_KEY");
}

TEST(StatusProtocol, Given_StatusResponseMessage_When_StatusResponseIsCreated_Then_StatusMatchesPayload)
{
    // Given
    const std::string jsonPayload = "{\"state\":\"CONNECTED\"}";
    const std::string channel = "d2p/status/d/DEVICE_KEY";
    const auto message = std::make_shared<wolkabout::Message>(jsonPayload, channel);

    // When
    const auto response = wolkabout::StatusProtocol::makeDeviceStatusResponse(message);

    // Then
    ASSERT_TRUE(response);
    ASSERT_EQ(response->getStatus(), wolkabout::DeviceStatusResponse::Status::CONNECTED);
}
