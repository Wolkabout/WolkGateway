#include "model/Message.h"
#include "protocol/json/JsonGatewaySubdeviceRegistrationProtocol.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

namespace
{
class JsonGatewaySubdeviceRegistrationProtocol : public ::testing::Test
{
public:
    void SetUp() override
    {
        protocol = std::unique_ptr<wolkabout::JsonGatewaySubdeviceRegistrationProtocol>(
          new wolkabout::JsonGatewaySubdeviceRegistrationProtocol());
    }

    void TearDown() override {}

    std::unique_ptr<wolkabout::JsonGatewaySubdeviceRegistrationProtocol> protocol;
};
}    // namespace

TEST_F(JsonGatewaySubdeviceRegistrationProtocol,
       Given_RegistrationRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string registrationRequestChannel = "d2p/register_subdevice_request/g/GATEWAY_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(registrationRequestChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", deviceKey);
}

TEST_F(JsonGatewaySubdeviceRegistrationProtocol,
       Given_RegistrationResponseChannel_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsValid)
{
    // Given
    const std::string registrationRespoonseChannel = "p2d/register_subdevice/g/GATEWAY_KEY/d/DEVICE_KEY_";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(registrationRespoonseChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY_", deviceKey);
}

TEST_F(JsonGatewaySubdeviceRegistrationProtocol,
       Given_ReregistrationRequestChannel_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsValid)
{
    // Given
    const std::string reregistrationRequestChannel = "p2d/reregister_subdevice/g/GATEWAY_KEY/d/__DEVICE_KEY";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(reregistrationRequestChannel);

    // Then
    ASSERT_EQ("__DEVICE_KEY", deviceKey);
}

TEST_F(JsonGatewaySubdeviceRegistrationProtocol,
       Given_RegistrationRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsRegistrationRequest)
{
    // Given
    const std::string registrationRequestChannel = "d2p/register_subdevice_request/g/GATEWAY_KEY";
    auto registrationRequestMessage = std::make_shared<wolkabout::Message>("", registrationRequestChannel);

    // When
    const bool isSubdeviceRegistrationRequest = protocol->isSubdeviceRegistrationRequest(*registrationRequestMessage);

    // Then
    ASSERT_TRUE(isSubdeviceRegistrationRequest);
}

TEST_F(JsonGatewaySubdeviceRegistrationProtocol,
       Given_RegistrationResponseMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsRegistrationResponse)
{
    // Given
    const std::string registrationResponseChannel = "p2d/register_subdevice_response/g/GATEWAY_KEY";
    auto registrationResponseMessage = std::make_shared<wolkabout::Message>("", registrationResponseChannel);

    // When
    const bool isSubdeviceRegistrationResponse =
      protocol->isSubdeviceRegistrationResponse(*registrationResponseMessage);

    // Then
    ASSERT_TRUE(isSubdeviceRegistrationResponse);
}

TEST_F(JsonGatewaySubdeviceRegistrationProtocol,
       Given_DeviceDeletionRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsDeviceDeletionRequest)
{
    // Given
    const std::string deviceDeletionRequestChannel = "d2p/delete_subdevice_request/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto deviceDeletionRequestMessage = std::make_shared<wolkabout::Message>("", deviceDeletionRequestChannel);

    // When
    const bool isDeviceDeletionRequest = protocol->isSubdeviceDeletionRequest(*deviceDeletionRequestMessage);

    // Then
    ASSERT_TRUE(isDeviceDeletionRequest);
}

TEST_F(JsonGatewaySubdeviceRegistrationProtocol,
       Given_DeviceDeletionResponseMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsDeviceDeletionResponse)
{
    // Given
    const std::string deviceDeletionResponseChannel = "p2d/delete_subdevice_response/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto deviceDeletionResponseMessage = std::make_shared<wolkabout::Message>("", deviceDeletionResponseChannel);

    // When
    const bool isDeviceDeletionResponse = protocol->isSubdeviceDeletionResponse(*deviceDeletionResponseMessage);

    // Then
    ASSERT_TRUE(isDeviceDeletionResponse);
}

TEST_F(JsonGatewaySubdeviceRegistrationProtocol,
       Given_MessageFromPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsFromPlatform)
{
    // Given
    const std::string registrationResponseChannel = "p2d/register_subdevice_response/g/GATEWAY_KEY";
    auto message = std::make_shared<wolkabout::Message>("", registrationResponseChannel);

    // When
    const bool isMessageFromPlatform = protocol->isMessageFromPlatform(*message);

    // Then
    ASSERT_TRUE(isMessageFromPlatform);
}

TEST_F(JsonGatewaySubdeviceRegistrationProtocol,
       Given_MessageToPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsToPlatform)
{
    // Given
    const std::string registrationResponseChannel = "d2p/register_subdevice_request/g/GATEWAY_KEY";
    auto message = std::make_shared<wolkabout::Message>("", registrationResponseChannel);

    // When
    const bool isMessageToPlatform = protocol->isMessageToPlatform(*message);

    // Then
    ASSERT_TRUE(isMessageToPlatform);
}

TEST_F(JsonGatewaySubdeviceRegistrationProtocol, VerifyDeviceTopics)
{
    std::vector<std::string> deviceTopics = protocol->getInboundDeviceChannels();

    ASSERT_THAT(deviceTopics, ::testing::ElementsAre("d2p/register_subdevice_request/d/#"));
}

TEST_F(JsonGatewaySubdeviceRegistrationProtocol, VerifyPlatformTopics)
{
    std::vector<std::string> platformTopics = protocol->getInboundPlatformChannels();

    ASSERT_THAT(platformTopics, ::testing::UnorderedElementsAre("p2d/register_subdevice_response/g/#",
                                                                "p2d/update_gateway_response/g/#",
                                                                "p2d/delete_subdevice_response/g/#"));
}
