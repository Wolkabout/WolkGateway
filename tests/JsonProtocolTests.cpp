#include "model/ActuatorGetCommand.h"
#include "model/ActuatorSetCommand.h"
#include "model/ActuatorStatus.h"
#include "model/Message.h"
#define private public
#define protected public
#include "protocol/json/JsonGatewayDataProtocol.h"
#undef protected
#undef private

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

TEST_F(JsonGatewayDataProtocol, Given_Channels_When_DeviceChannelsAreRequested_Then_DeviceChannelsMatchChannels)
{
    // Given
    const std::vector<std::string> channels{"d2p/sensor_reading/d/+/r/#", "d2p/events/d/+/r/#",
                                            "d2p/actuator_status/d/+/r/#", "d2p/configuration_get/d/#"};

    // When
    const auto deviceChannels = protocol->getInboundChannels();

    // Then
    for (const auto& channel : channels)
    {
        auto it = std::find(deviceChannels.begin(), deviceChannels.end(), channel);
        ASSERT_TRUE(it != deviceChannels.end());
    }
}
