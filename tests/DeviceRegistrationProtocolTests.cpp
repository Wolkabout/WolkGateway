#include "model/Message.h"
#include "protocol/json/JsonGatewayDeviceRegistrationProtocol.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace
{
class JsonGatewayDeviceRegistrationProtocol : public ::testing::Test
{
public:
    void SetUp() override
    {
        protocol = std::unique_ptr<wolkabout::JsonGatewayDeviceRegistrationProtocol>(
          new wolkabout::JsonGatewayDeviceRegistrationProtocol());
    }

    void TearDown() override {}

    std::unique_ptr<wolkabout::JsonGatewayDeviceRegistrationProtocol> protocol;
};
}    // namespace

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_RegistrationRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string registrationRequestChannel = "d2p/register_device/g/GATEWAY_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(registrationRequestChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", deviceKey);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_RegistrationRequestChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsValid)
{
    // Given
    const std::string registrationRequestChannel = "d2p/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(registrationRequestChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_RegistrationResponseChannel_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsValid)
{
    // Given
    const std::string registrationRespoonseChannel = "p2d/register_device/g/GATEWAY_KEY/d/DEVICE_KEY_";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(registrationRespoonseChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY_", deviceKey);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_ReregistrationRequestChannel_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsValid)
{
    // Given
    const std::string reregistrationRequestChannel = "p2d/reregister_device/g/GATEWAY_KEY/d/__DEVICE_KEY";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(reregistrationRequestChannel);

    // Then
    ASSERT_EQ("__DEVICE_KEY", deviceKey);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_ReregistrationResponseChannel_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsValid)
{
    // Given
    const std::string reregistrationResponseChannel = "d2p/reregister_device/g/GATEWAY_KEY/d/__DEVICE_KEY__";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(reregistrationResponseChannel);

    // Then
    ASSERT_EQ("__DEVICE_KEY__", deviceKey);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_RegistrationRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsRegistrationRequest)
{
    // Given
    const std::string registrationRequestChannel = "d2p/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto registrationRequestMessage = std::make_shared<wolkabout::Message>("", registrationRequestChannel);

    // When
    const bool isRegistrationRequest = protocol->isRegistrationRequest(*registrationRequestMessage);

    // Then
    ASSERT_TRUE(isRegistrationRequest);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_RegistrationResponseMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsRegistrationResponse)
{
    // Given
    const std::string registrationResponseChannel = "p2d/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto registrationResponseMessage = std::make_shared<wolkabout::Message>("", registrationResponseChannel);

    // When
    const bool isRegistrationResponse = protocol->isRegistrationResponse(*registrationResponseMessage);

    // Then
    ASSERT_TRUE(isRegistrationResponse);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_ReregistrationRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsReregistrationRequest)
{
    // Given
    const std::string reregistrationRequestChannel = "d2p/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto reregistrationRequestMessage = std::make_shared<wolkabout::Message>("", reregistrationRequestChannel);

    // When
    const bool isReregistrationRequest = protocol->isRegistrationRequest(*reregistrationRequestMessage);

    // Then
    ASSERT_TRUE(isReregistrationRequest);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_ReregistrationResponseMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsReregistrationResponse)
{
    // Given
    const std::string registrationResponseChannel = "p2d/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto registrationResponseMessage = std::make_shared<wolkabout::Message>("", registrationResponseChannel);

    // When
    const bool isReregistrationResponse = protocol->isRegistrationResponse(*registrationResponseMessage);

    // Then
    ASSERT_TRUE(isReregistrationResponse);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_DeviceDeletionRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsDeviceDeletionRequest)
{
    // Given
    const std::string deviceDeletionRequestChannel = "d2p/delete_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto deviceDeletionRequestMessage = std::make_shared<wolkabout::Message>("", deviceDeletionRequestChannel);

    // When
    const bool isDeviceDeletionRequest = protocol->isDeviceDeletionRequest(*deviceDeletionRequestMessage);

    // Then
    ASSERT_TRUE(isDeviceDeletionRequest);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_DeviceDeletionResponseMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsDeviceDeletionResponse)
{
    // Given
    const std::string deviceDeletionResponseChannel = "p2d/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto deviceDeletionResponseMessage = std::make_shared<wolkabout::Message>("", deviceDeletionResponseChannel);

    // When
    const bool isDeviceDeletionResponse = protocol->isRegistrationResponse(*deviceDeletionResponseMessage);

    // Then
    ASSERT_TRUE(isDeviceDeletionResponse);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_MessageFromPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsFromPlatform)
{
    // Given
    const std::string registrationResponseChannel = "p2d/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto message = std::make_shared<wolkabout::Message>("", registrationResponseChannel);

    // When
    const bool isMessageFromPlatform = protocol->isMessageFromPlatform(*message);

    // Then
    ASSERT_TRUE(isMessageFromPlatform);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol,
       Given_MessageToPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsToPlatform)
{
    // Given
    const std::string registrationResponseChannel = "d2p/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto message = std::make_shared<wolkabout::Message>("", registrationResponseChannel);

    // When
    const bool isMessageToPlatform = protocol->isMessageToPlatform(*message);

    // Then
    ASSERT_TRUE(isMessageToPlatform);
}

TEST_F(JsonGatewayDeviceRegistrationProtocol, VerifyDeviceTopics)
{
    std::vector<std::string> deviceTopics = protocol->getInboundDeviceChannels();

    ASSERT_THAT(deviceTopics, ::testing::ElementsAre("d2p/register_device/d/#"));
}

TEST_F(JsonGatewayDeviceRegistrationProtocol, VerifyPlatformTopics)
{
    std::vector<std::string> platformTopics = protocol->getInboundPlatformChannels();

    ASSERT_THAT(platformTopics, ::testing::UnorderedElementsAre("p2d/register_device/g/#", "p2d/reregister_device/g/#",
                                                                "p2d/delete_device/g/#"));
}
