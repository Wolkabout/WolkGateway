#include "connectivity/json/JsonProtocol.h"
#include "model/ActuatorGetCommand.h"
#include "model/ActuatorSetCommand.h"
#include "model/ActuatorStatus.h"
#include "model/Message.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(JsonProtocol, Given_Name_When_ProtocolNameIsRequested_Then_NameIsEqualToProtocolName)
{
    // Given
    const std::string name = "JsonProtocol";

    // When
    const std::string protocolName = wolkabout::JsonProtocol::getName();

    // Then
    ASSERT_EQ(name, protocolName);
}

TEST(JsonProtocol,
     Given_SensorReadingChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string sensorReadingChannel = "d2p/sensor_reading/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(sensorReadingChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(JsonProtocol, Given_EventChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string eventChannel = "d2p/events/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(eventChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(JsonProtocol,
     Given_ActuatorStatusChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string actuatorStatusChannel = "d2p/actuator_status/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(actuatorStatusChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(JsonProtocol,
     Given_ConfigurationSetResponseChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string configurationSetResponseChannel = "d2p/configuration_set/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(configurationSetResponseChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(JsonProtocol,
     Given_ConfigurationGetResponseChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string configurationGetResponseChannel = "d2p/configuration_get/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(configurationGetResponseChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(JsonProtocol,
     Given_ActuationSetRequestChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string actuationSetChannel = "p2d/actuation_set/g/GATEWAY_KEY/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(actuationSetChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(JsonProtocol,
     Given_ActuationSetRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string actuationSetChannel = "p2d/actuation_set/g/GATEWAY_KEY/";

    // When
    const std::string gateway = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(actuationSetChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", gateway);
}

TEST(JsonProtocol,
     Given_ActuationGetRequestChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string actuationGetChannel = "p2d/actuation_get/g/GATEWAY_KEY/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(actuationGetChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(JsonProtocol,
     Given_ActuationGetRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string actuationGetChannel = "p2d/actuation_get/g/GATEWAY_KEY/";

    // When
    const std::string gateway = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(actuationGetChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", gateway);
}

TEST(JsonProtocol,
     Given_ConfigurationSetRequestChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string configurationSetChannel = "p2d/configuration_set/g/GATEWAY_KEY/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(configurationSetChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(
  JsonProtocol,
  Given_ConfigurationSetRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string configurationSetChannel = "p2d/configuration_set/g/GATEWAY_KEY/";

    // When
    const std::string gateway = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(configurationSetChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", gateway);
}

TEST(JsonProtocol,
     Given_ConfigurationGetRequestChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string configurationGetChannel = "p2d/configuration_get/g/GATEWAY_KEY/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(configurationGetChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(
  JsonProtocol,
  Given_ConfigurationGetRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string configurationGetChannel = "p2d/configuration_get/g/GATEWAY_KEY/";

    // When
    const std::string gateway = wolkabout::JsonProtocol::extractDeviceKeyFromChannel(configurationGetChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", gateway);
}

TEST(JsonProtocol, Given_ValidRefChannelForGateway_When_ReferenceIsExtracted_Then_ExtractedReferenceIsEqualToReference)
{
    // Given
    const std::string anyChannel = "p2d/configuration_get/g/GATEWAY_KEY/r/REF";

    // When
    const std::string reference = wolkabout::JsonProtocol::extractReferenceFromChannel(anyChannel);

    // Then
    ASSERT_EQ("REF", reference);
}

TEST(JsonProtocol,
     Given_RefChannelWithoutRefPrefixForGateway_When_ReferenceIsExtracted_Then_ExtractedReferenceIsEqualToEmpty)
{
    // Given
    const std::string anyChannel = "p2d/configuration_get/g/GATEWAY_KEY/REF";

    // When
    const std::string reference = wolkabout::JsonProtocol::extractReferenceFromChannel(anyChannel);

    // Then
    ASSERT_EQ("", reference);
}

TEST(
  JsonProtocol,
  Given_ValidRefChannelWithMultilevelRefForGateway_When_ReferenceIsExtracted_Then_ExtractedReferenceIsEqualToReference)
{
    // Given
    const std::string anyChannel = "p2d/configuration_get/g/GATEWAY_KEY/r/REF/p2d/actuation_set";

    // When
    const std::string reference = wolkabout::JsonProtocol::extractReferenceFromChannel(anyChannel);

    // Then
    ASSERT_EQ("REF/p2d/actuation_set", reference);
}

TEST(JsonProtocol, Given_GatewayDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToDeviceChannel)
{
    // Given
    const std::string channel = "p2d/configuration_get/g/GATEWAY_KEY/d/DEVICE_KEY/r/REF";

    // When
    const std::string routedChannel = wolkabout::JsonProtocol::routePlatformToDeviceMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("p2d/configuration_get/d/DEVICE_KEY/r/REF", routedChannel);
}

TEST(JsonProtocol, Given_InvalidGatewayDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToEmpty)
{
    // Given
    const std::string channel = "p2d/configuration_get/GATEWAY_KEY/d/DEVICE_KEY/r/REF";

    // When
    const std::string routedChannel = wolkabout::JsonProtocol::routePlatformToDeviceMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("", routedChannel);
}

TEST(JsonProtocol, Given_GatewayChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToGWDeviceChannel)
{
    // Given
    const std::string channel = "p2d/configuration_get/g/GATEWAY_KEY/r/REF";

    // When
    const std::string routedChannel = wolkabout::JsonProtocol::routePlatformToGatewayMessage(channel);

    // Then
    ASSERT_EQ("p2d/configuration_get/d/GATEWAY_KEY/r/REF", routedChannel);
}

TEST(JsonProtocol, Given_InvalidGatewayChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToEmpty)
{
    // Given
    const std::string channel = "p2d/configuration_get/GATEWAY_KEY/r/REF";

    // When
    const std::string routedChannel = wolkabout::JsonProtocol::routePlatformToGatewayMessage(channel);

    // Then
    ASSERT_EQ("", routedChannel);
}

TEST(JsonProtocol, Given_DeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToGatewayDeviceChannel)
{
    // Given
    const std::string channel = "d2p/configuration_get/d/DEVICE_KEY/r/REF";

    // When
    const std::string routedChannel = wolkabout::JsonProtocol::routeDeviceToPlatformMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("d2p/configuration_get/g/GATEWAY_KEY/d/DEVICE_KEY/r/REF", routedChannel);
}

TEST(JsonProtocol, Given_InvalidDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToEmpty)
{
    // Given
    const std::string channel = "d2p/configuration_get/DEVICE_KEY/r/REF";

    // When
    const std::string routedChannel = wolkabout::JsonProtocol::routeDeviceToPlatformMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("", routedChannel);
}

TEST(JsonProtocol, Given_GWDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToGatewayChannel)
{
    // Given
    const std::string channel = "d2p/configuration_get/d/GATEWAY_KEY/r/REF";

    // When
    const std::string routedChannel = wolkabout::JsonProtocol::routeGatewayToPlatformMessage(channel);

    // Then
    ASSERT_EQ("d2p/configuration_get/g/GATEWAY_KEY/r/REF", routedChannel);
}

TEST(JsonProtocol, Given_InvalidGWDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToEmpty)
{
    // Given
    const std::string channel = "d2p/configuration_get/GATEWAY_KEY/r/REF";

    // When
    const std::string routedChannel = wolkabout::JsonProtocol::routeGatewayToPlatformMessage(channel);

    // Then
    ASSERT_EQ("", routedChannel);
}

TEST(JsonProtocol, Given_MessageFromPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsFromPlatform)
{
    // Given
    const std::string actuationRequestChannel = "p2d/actuation_set/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isMessageFromPlatform = wolkabout::JsonProtocol::isMessageFromPlatform(actuationRequestChannel);

    // Then
    ASSERT_TRUE(isMessageFromPlatform);
}

TEST(JsonProtocol, Given_MessageFromDevice_When_MessageDirectionIsChecked_Then_MessageDirectionDoesNotEqualFromPlatform)
{
    // Given
    const std::string actuationStatusChannel = "d2p/actuation_status/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isMessageFromPlatform = wolkabout::JsonProtocol::isMessageFromPlatform(actuationStatusChannel);

    // Then
    ASSERT_FALSE(isMessageFromPlatform);
}

TEST(JsonProtocol, Given_MessageToPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsToPlatform)
{
    // Given
    const std::string actuationStatusChannel = "d2p/actuation_status/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isMessageToPlatform = wolkabout::JsonProtocol::isMessageToPlatform(actuationStatusChannel);

    // Then
    ASSERT_TRUE(isMessageToPlatform);
}

TEST(JsonProtocol, Given_MessageToDevice_When_MessageDirectionIsChecked_Then_MessageDirectionDoesNotEqualToPlatform)
{
    // Given
    const std::string actuationRequestChannel = "p2d/actuation_set/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isMessageToPlatform = wolkabout::JsonProtocol::isMessageToPlatform(actuationRequestChannel);

    // Then
    ASSERT_FALSE(isMessageToPlatform);
}

TEST(JsonProtocol, Given_ActuationSetRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsActuationSetRequest)
{
    // Given
    const std::string actuationSetRequestChannel = "p2d/actuator_set/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isActuationSetRequest = wolkabout::JsonProtocol::isActuatorSetMessage(actuationSetRequestChannel);

    // Then
    ASSERT_TRUE(isActuationSetRequest);
}

TEST(JsonProtocol, Given_ActuationGetRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsActuationGetRequest)
{
    // Given
    const std::string actuationGetRequestChannel = "p2d/actuator_get/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isActuationGetRequest = wolkabout::JsonProtocol::isActuatorGetMessage(actuationGetRequestChannel);

    // Then
    ASSERT_TRUE(isActuationGetRequest);
}

TEST(JsonProtocol, Given_ActuatorStatus_When_MessageIsCreated_Then_MessageChannelMatchesReference)
{
    // Given
    wolkabout::ActuatorStatus status{"VALUE", "REF", wolkabout::ActuatorStatus::State::READY};

    // When
    const auto message = wolkabout::JsonProtocol::make("GATEWAY_KEY", status);

    // Then
    ASSERT_TRUE(message != nullptr);
    ASSERT_EQ(message->getChannel(), "d2p/actuator_status/g/GATEWAY_KEY/r/REF");
}

TEST(JsonProtocol,
     Given_ActuatorSetMessage_When_ActuatorSetCommandIsCreated_Then_ValueMatchesPayloadAndReferenceMatchesChannel)
{
    // Given
    const std::string jsonPayload = "{\"value\":\"TEST_VALUE\"}";
    const std::string channel = "p2d/actuator_set/g/GATEWAY_KEY/r/REF";
    const auto message = std::make_shared<wolkabout::Message>(jsonPayload, channel);

    // When
    wolkabout::ActuatorSetCommand command;
    const bool result = wolkabout::JsonProtocol::fromMessage(message, command);

    // Then
    ASSERT_TRUE(result);
    ASSERT_EQ(command.getValue(), "TEST_VALUE");
    ASSERT_EQ(command.getReference(), "REF");
}

TEST(JsonProtocol, Given_ActuatorGetMessage_When_ActuatorGetCommandIsCreated_Then_ReferenceMatchesChannel)
{
    // Given
    const std::string channel = "p2d/actuator_set/g/GATEWAY_KEY/r/REF";
    const auto message = std::make_shared<wolkabout::Message>("", channel);

    // When
    wolkabout::ActuatorGetCommand command;
    const bool result = wolkabout::JsonProtocol::fromMessage(message, command);

    // Then
    ASSERT_TRUE(result);
    ASSERT_EQ(command.getReference(), "REF");
}

TEST(JsonProtocol, Given_Channels_When_DeviceChannelsAreRequested_Then_DeviceChannelsMatchChannels)
{
    // Given
    const std::vector<std::string> channels{"d2p/sensor_reading/#", "d2p/events/#", "d2p/actuator_status/#"};

    // When
    const auto deviceChannels = wolkabout::JsonProtocol::getDeviceTopics();

    // Then
    for (const auto& channel : channels)
    {
        auto it = std::find(deviceChannels.begin(), deviceChannels.end(), channel);
        ASSERT_TRUE(it != deviceChannels.end());
    }
}

TEST(JsonProtocol, Given_Channels_When_PlatformChannelsAreRequested_Then_PlatformChannelsMatchChannels)
{
    // Given
    const std::vector<std::string> channels{"p2d/actuator_set/#", "p2d/actuator_get/#"};

    // When
    const auto platformChannels = wolkabout::JsonProtocol::getPlatformTopics();

    // Then
    for (const auto& channel : channels)
    {
        auto it = std::find(platformChannels.begin(), platformChannels.end(), channel);
        ASSERT_TRUE(it != platformChannels.end());
    }
}
