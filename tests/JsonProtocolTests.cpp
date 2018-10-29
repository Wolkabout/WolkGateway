#include "model/ActuatorGetCommand.h"
#include "model/ActuatorSetCommand.h"
#include "model/ActuatorStatus.h"
#include "model/Message.h"
#include "protocol/json/JsonGatewayDataProtocol.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace
{
class JsonGatewayDataProtocol : public ::testing::Test
{
public:
    void SetUp() override
    {
        protocol = std::unique_ptr<wolkabout::GatewayDataProtocol>(new wolkabout::JsonGatewayDataProtocol());
    }

    void TearDown() override {}

    std::unique_ptr<wolkabout::GatewayDataProtocol> protocol;
};
}    // namespace

TEST_F(JsonGatewayDataProtocol, Given_Name_When_ProtocolNameIsRequested_Then_NameIsEqualToProtocolName)
{
    // Given
    const std::string name = "JsonProtocol";

    // When
    const std::string protocolName = protocol->getName();

    // Then
    ASSERT_EQ(name, protocolName);
}

TEST_F(JsonGatewayDataProtocol,
       Given_SensorReadingChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string sensorReadingChannel = "d2p/sensor_reading/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(sensorReadingChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(JsonGatewayDataProtocol,
       Given_EventChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string eventChannel = "d2p/events/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(eventChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(JsonGatewayDataProtocol,
       Given_ActuatorStatusChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string actuatorStatusChannel = "d2p/actuator_status/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(actuatorStatusChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(
  JsonGatewayDataProtocol,
  Given_ConfigurationSetResponseChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string configurationSetResponseChannel = "d2p/configuration_set/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(configurationSetResponseChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(
  JsonGatewayDataProtocol,
  Given_ConfigurationGetResponseChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string configurationGetResponseChannel = "d2p/configuration_get/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(configurationGetResponseChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(JsonGatewayDataProtocol,
       Given_ActuationSetRequestChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string actuationSetChannel = "p2d/actuation_set/g/GATEWAY_KEY/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(actuationSetChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(JsonGatewayDataProtocol,
       Given_ActuationSetRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string actuationSetChannel = "p2d/actuation_set/g/GATEWAY_KEY/";

    // When
    const std::string gateway = protocol->extractDeviceKeyFromChannel(actuationSetChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", gateway);
}

TEST_F(JsonGatewayDataProtocol,
       Given_ActuationGetRequestChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string actuationGetChannel = "p2d/actuation_get/g/GATEWAY_KEY/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(actuationGetChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(JsonGatewayDataProtocol,
       Given_ActuationGetRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string actuationGetChannel = "p2d/actuation_get/g/GATEWAY_KEY/";

    // When
    const std::string gateway = protocol->extractDeviceKeyFromChannel(actuationGetChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", gateway);
}

TEST_F(
  JsonGatewayDataProtocol,
  Given_ConfigurationSetRequestChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string configurationSetChannel = "p2d/configuration_set/g/GATEWAY_KEY/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(configurationSetChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(
  JsonGatewayDataProtocol,
  Given_ConfigurationSetRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string configurationSetChannel = "p2d/configuration_set/g/GATEWAY_KEY/";

    // When
    const std::string gateway = protocol->extractDeviceKeyFromChannel(configurationSetChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", gateway);
}

TEST_F(
  JsonGatewayDataProtocol,
  Given_ConfigurationGetRequestChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToDeviceKey)
{
    // Given
    const std::string configurationGetChannel = "p2d/configuration_get/g/GATEWAY_KEY/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(configurationGetChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(
  JsonGatewayDataProtocol,
  Given_ConfigurationGetRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string configurationGetChannel = "p2d/configuration_get/g/GATEWAY_KEY/";

    // When
    const std::string gateway = protocol->extractDeviceKeyFromChannel(configurationGetChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", gateway);
}

TEST_F(JsonGatewayDataProtocol,
       Given_ValidRefChannelForGateway_When_ReferenceIsExtracted_Then_ExtractedReferenceIsEqualToReference)
{
    // Given
    const std::string anyChannel = "p2d/configuration_get/g/GATEWAY_KEY/r/REF";

    // When
    const std::string reference = protocol->extractReferenceFromChannel(anyChannel);

    // Then
    ASSERT_EQ("REF", reference);
}

TEST_F(JsonGatewayDataProtocol,
       Given_RefChannelWithoutRefPrefixForGateway_When_ReferenceIsExtracted_Then_ExtractedReferenceIsEqualToEmpty)
{
    // Given
    const std::string anyChannel = "p2d/configuration_get/g/GATEWAY_KEY/REF";

    // When
    const std::string reference = protocol->extractReferenceFromChannel(anyChannel);

    // Then
    ASSERT_EQ("", reference);
}

TEST_F(
  JsonGatewayDataProtocol,
  Given_ValidRefChannelWithMultilevelRefForGateway_When_ReferenceIsExtracted_Then_ExtractedReferenceIsEqualToReference)
{
    // Given
    const std::string anyChannel = "p2d/configuration_get/g/GATEWAY_KEY/r/REF/p2d/actuation_set";

    // When
    const std::string reference = protocol->extractReferenceFromChannel(anyChannel);

    // Then
    ASSERT_EQ("REF/p2d/actuation_set", reference);
}

TEST_F(JsonGatewayDataProtocol,
       Given_GatewayDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToDeviceChannel)
{
    // Given
    const std::string channel = "p2d/configuration_get/g/GATEWAY_KEY/d/DEVICE_KEY/r/REF";

    // When
    const std::string routedChannel = protocol->routePlatformToDeviceMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("p2d/configuration_get/d/DEVICE_KEY/r/REF", routedChannel);
}

TEST_F(JsonGatewayDataProtocol, Given_InvalidGatewayDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToEmpty)
{
    // Given
    const std::string channel = "p2d/configuration_get/GATEWAY_KEY/d/DEVICE_KEY/r/REF";

    // When
    const std::string routedChannel = protocol->routePlatformToDeviceMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("", routedChannel);
}

TEST_F(JsonGatewayDataProtocol, Given_GatewayChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToGWDeviceChannel)
{
    // Given
    const std::string channel = "p2d/configuration_get/g/GATEWAY_KEY/r/REF";

    // When
    const std::string routedChannel = protocol->routePlatformToGatewayMessage(channel);

    // Then
    ASSERT_EQ("p2d/configuration_get/d/GATEWAY_KEY/r/REF", routedChannel);
}

TEST_F(JsonGatewayDataProtocol, Given_InvalidGatewayChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToEmpty)
{
    // Given
    const std::string channel = "p2d/configuration_get/GATEWAY_KEY/r/REF";

    // When
    const std::string routedChannel = protocol->routePlatformToGatewayMessage(channel);

    // Then
    ASSERT_EQ("", routedChannel);
}

TEST_F(JsonGatewayDataProtocol,
       Given_DeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToGatewayDeviceChannel)
{
    // Given
    const std::string channel = "d2p/configuration_get/d/DEVICE_KEY/r/REF";

    // When
    const std::string routedChannel = protocol->routeDeviceToPlatformMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("d2p/configuration_get/g/GATEWAY_KEY/d/DEVICE_KEY/r/REF", routedChannel);
}

TEST_F(JsonGatewayDataProtocol, Given_InvalidDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToEmpty)
{
    // Given
    const std::string channel = "d2p/configuration_get/DEVICE_KEY/r/REF";

    // When
    const std::string routedChannel = protocol->routeDeviceToPlatformMessage(channel, "GATEWAY_KEY");

    // Then
    ASSERT_EQ("", routedChannel);
}

TEST_F(JsonGatewayDataProtocol, Given_GWDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToGatewayChannel)
{
    // Given
    const std::string channel = "d2p/configuration_get/d/GATEWAY_KEY/r/REF";

    // When
    const std::string routedChannel = protocol->routeGatewayToPlatformMessage(channel);

    // Then
    ASSERT_EQ("d2p/configuration_get/g/GATEWAY_KEY/r/REF", routedChannel);
}

TEST_F(JsonGatewayDataProtocol, Given_InvalidGWDeviceChannel_When_ChannelIsRouted_Then_RoutedChannelIsEqualToEmpty)
{
    // Given
    const std::string channel = "d2p/configuration_get/GATEWAY_KEY/r/REF";

    // When
    const std::string routedChannel = protocol->routeGatewayToPlatformMessage(channel);

    // Then
    ASSERT_EQ("", routedChannel);
}

TEST_F(JsonGatewayDataProtocol,
       Given_MessageFromPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsFromPlatform)
{
    // Given
    const std::string actuationRequestChannel = "p2d/actuation_set/g/GATEWAY_KEY/d/DEVICE_KEY";
    const auto message = std::make_shared<wolkabout::Message>("", actuationRequestChannel);

    // When
    const bool isMessageFromPlatform = protocol->isMessageFromPlatform(*message);

    // Then
    ASSERT_TRUE(isMessageFromPlatform);
}

TEST_F(JsonGatewayDataProtocol,
       Given_MessageFromDevice_When_MessageDirectionIsChecked_Then_MessageDirectionDoesNotEqualFromPlatform)
{
    // Given
    const std::string actuationStatusChannel = "d2p/actuation_status/g/GATEWAY_KEY/d/DEVICE_KEY";
    const auto message = std::make_shared<wolkabout::Message>("", actuationStatusChannel);

    // When
    const bool isMessageFromPlatform = protocol->isMessageFromPlatform(*message);

    // Then
    ASSERT_FALSE(isMessageFromPlatform);
}

TEST_F(JsonGatewayDataProtocol,
       Given_MessageToPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsToPlatform)
{
    // Given
    const std::string actuationStatusChannel = "d2p/actuation_status/g/GATEWAY_KEY/d/DEVICE_KEY";
    const auto message = std::make_shared<wolkabout::Message>("", actuationStatusChannel);

    // When
    const bool isMessageToPlatform = protocol->isMessageToPlatform(*message);

    // Then
    ASSERT_TRUE(isMessageToPlatform);
}

TEST_F(JsonGatewayDataProtocol,
       Given_MessageToDevice_When_MessageDirectionIsChecked_Then_MessageDirectionDoesNotEqualToPlatform)
{
    // Given
    const std::string actuationRequestChannel = "p2d/actuation_set/g/GATEWAY_KEY/d/DEVICE_KEY";
    const auto message = std::make_shared<wolkabout::Message>("", actuationRequestChannel);

    // When
    const bool isMessageToPlatform = protocol->isMessageToPlatform(*message);

    // Then
    ASSERT_FALSE(isMessageToPlatform);
}

TEST_F(JsonGatewayDataProtocol,
       Given_ActuationSetRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsActuationSetRequest)
{
    // Given
    const std::string actuationSetRequestChannel = "p2d/actuator_set/g/GATEWAY_KEY/d/DEVICE_KEY";
    const auto message = std::make_shared<wolkabout::Message>("", actuationSetRequestChannel);

    // When
    const bool isActuationSetRequest = protocol->isActuatorSetMessage(*message);

    // Then
    ASSERT_TRUE(isActuationSetRequest);
}

TEST_F(JsonGatewayDataProtocol,
       Given_ActuationGetRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsActuationGetRequest)
{
    // Given
    const std::string actuationGetRequestChannel = "p2d/actuator_get/g/GATEWAY_KEY/d/DEVICE_KEY";
    const auto message = std::make_shared<wolkabout::Message>("", actuationGetRequestChannel);

    // When
    const bool isActuationGetRequest = protocol->isActuatorGetMessage(*message);

    // Then
    ASSERT_TRUE(isActuationGetRequest);
}

TEST_F(JsonGatewayDataProtocol, Given_ActuatorStatus_When_MessageIsCreated_Then_MessageChannelMatchesReference)
{
    // Given
    wolkabout::ActuatorStatus status{"VALUE", "REF", wolkabout::ActuatorStatus::State::READY};

    // When
    const auto message = protocol->makeMessage("GATEWAY_KEY", status);

    // Then
    ASSERT_TRUE(message != nullptr);
    ASSERT_EQ(message->getChannel(), "d2p/actuator_status/g/GATEWAY_KEY/r/REF");
}

TEST_F(JsonGatewayDataProtocol,
       Given_ActuatorSetMessage_When_ActuatorSetCommandIsCreated_Then_ValueMatchesPayloadAndReferenceMatchesChannel)
{
    // Given
    const std::string jsonPayload = "{\"value\":\"TEST_VALUE\"}";
    const std::string channel = "p2d/actuator_set/g/GATEWAY_KEY/r/REF";
    const auto message = std::make_shared<wolkabout::Message>(jsonPayload, channel);

    // When
    const auto command = protocol->makeActuatorSetCommand(*message);

    // Then
    ASSERT_TRUE(command != nullptr);
    ASSERT_EQ(command->getValue(), "TEST_VALUE");
    ASSERT_EQ(command->getReference(), "REF");
}

TEST_F(JsonGatewayDataProtocol, Given_ActuatorGetMessage_When_ActuatorGetCommandIsCreated_Then_ReferenceMatchesChannel)
{
    // Given
    const std::string channel = "p2d/actuator_set/g/GATEWAY_KEY/r/REF";
    const auto message = std::make_shared<wolkabout::Message>("", channel);

    // When
    const auto command = protocol->makeActuatorGetCommand(*message);

    // Then
    ASSERT_TRUE(command != nullptr);
    ASSERT_EQ(command->getReference(), "REF");
}

TEST_F(JsonGatewayDataProtocol, Given_Channels_When_DeviceChannelsAreRequested_Then_DeviceChannelsMatchChannels)
{
    // Given
    const std::vector<std::string> channels{"d2p/sensor_reading/d/+/r/#", "d2p/events/d/+/r/#",
                                            "d2p/actuator_status/d/+/r/#", "d2p/configuration_get/d/#"};

    // When
    const auto deviceChannels = protocol->getInboundDeviceChannels();

    // Then
    for (const auto& channel : channels)
    {
        auto it = std::find(deviceChannels.begin(), deviceChannels.end(), channel);
        ASSERT_TRUE(it != deviceChannels.end());
    }
}

TEST_F(JsonGatewayDataProtocol, Given_Channels_When_PlatformChannelsAreRequested_Then_PlatformChannelsMatchChannels)
{
    // Given
    const std::vector<std::string> channels{"p2d/actuator_set/g/#", "p2d/actuator_get/g/#", "p2d/configuration_set/g/#",
                                            "p2d/configuration_get/g/#"};

    // When
    const auto platformChannels = protocol->getInboundPlatformChannels();

    // Then
    for (const auto& channel : channels)
    {
        auto it = std::find(platformChannels.begin(), platformChannels.end(), channel);
        ASSERT_TRUE(it != platformChannels.end());
    }
}
