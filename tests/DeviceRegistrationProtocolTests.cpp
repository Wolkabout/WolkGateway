#include "connectivity/json/DeviceRegistrationProtocol.h"
#include "model/Message.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <string>
#include <vector>

TEST(DeviceRegistrationProtocol,
     Given_RegistrationRequestChannelForGateway_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsEqualToGatewayKey)
{
    // Given
    const std::string registrationRequestChannel = "d2p/register_device/g/GATEWAY_KEY/";

    // When
    const std::string deviceKey =
      wolkabout::DeviceRegistrationProtocol::extractDeviceKeyFromChannel(registrationRequestChannel);

    // Then
    ASSERT_EQ("GATEWAY_KEY", deviceKey);
}

TEST(DeviceRegistrationProtocol,
     Given_RegistrationRequestChannelForDevice_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsValid)
{
    // Given
    const std::string registrationRequestChannel = "d2p/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const std::string deviceKey =
      wolkabout::DeviceRegistrationProtocol::extractDeviceKeyFromChannel(registrationRequestChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
}

TEST(DeviceRegistrationProtocol,
     Given_RegistrationResponseChannel_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsValid)
{
    // Given
    const std::string registrationRespoonseChannel = "p2d/register_device/g/GATEWAY_KEY/d/DEVICE_KEY_";

    // When
    const std::string deviceKey =
      wolkabout::DeviceRegistrationProtocol::extractDeviceKeyFromChannel(registrationRespoonseChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY_", deviceKey);
}

TEST(DeviceRegistrationProtocol,
     Given_ReregistrationRequestChannel_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsValid)
{
    // Given
    const std::string reregistrationRequestChannel = "p2d/reregister_device/g/GATEWAY_KEY/d/__DEVICE_KEY";

    // When
    const std::string deviceKey =
      wolkabout::DeviceRegistrationProtocol::extractDeviceKeyFromChannel(reregistrationRequestChannel);

    // Then
    ASSERT_EQ("__DEVICE_KEY", deviceKey);
}

TEST(DeviceRegistrationProtocol,
     Given_ReregistrationResponseChannel_When_DeviceKeyIsExtracted_Then_ExtractedDeviceKeyIsValid)
{
    // Given
    const std::string reregistrationResponseChannel = "d2p/reregister_device/g/GATEWAY_KEY/d/__DEVICE_KEY__";

    // When
    const std::string deviceKey =
      wolkabout::DeviceRegistrationProtocol::extractDeviceKeyFromChannel(reregistrationResponseChannel);

    // Then
    ASSERT_EQ("__DEVICE_KEY__", deviceKey);
}

TEST(DeviceRegistrationProtocol,
     Given_RegistrationRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsRegistrationRequest)
{
    // Given
    const std::string registrationRequestChannel = "d2p/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto registrationRequestMessage = std::make_shared<wolkabout::Message>("", registrationRequestChannel);

    // When
    const bool isRegistrationRequest =
      wolkabout::DeviceRegistrationProtocol::isRegistrationRequest(registrationRequestMessage);

    // Then
    ASSERT_TRUE(isRegistrationRequest);
}

TEST(DeviceRegistrationProtocol,
     Given_RegistrationResponseMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsRegistrationResponse)
{
    // Given
    const std::string registrationResponseChannel = "p2d/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto registrationResponseMessage = std::make_shared<wolkabout::Message>("", registrationResponseChannel);

    // When
    const bool isRegistrationResponse =
      wolkabout::DeviceRegistrationProtocol::isRegistrationResponse(registrationResponseMessage);

    // Then
    ASSERT_TRUE(isRegistrationResponse);
}

TEST(DeviceRegistrationProtocol,
     Given_ReregistrationRequestMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsReregistrationRequest)
{
    // Given
    const std::string reregistrationRequestChannel = "d2p/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto reregistrationRequestMessage = std::make_shared<wolkabout::Message>("", reregistrationRequestChannel);

    // When
    const bool isReregistrationRequest =
      wolkabout::DeviceRegistrationProtocol::isRegistrationRequest(reregistrationRequestMessage);

    // Then
    ASSERT_TRUE(isReregistrationRequest);
}

TEST(DeviceRegistrationProtocol,
     Given_ReregistrationResponseMessage_When_MessageTypeIsChecked_Then_MessageTypeEqualsReregistrationResponse)
{
    // Given
    const std::string registrationResponseChannel = "p2d/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";
    auto registrationResponseMessage = std::make_shared<wolkabout::Message>("", registrationResponseChannel);

    // When
    const bool isReregistrationResonse =
      wolkabout::DeviceRegistrationProtocol::isRegistrationResponse(registrationResponseMessage);

    // Then
    ASSERT_TRUE(isReregistrationResonse);
}

TEST(DeviceRegistrationProtocol,
     Given_MessageFromPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsFromPlatform)
{
    // Given
    const std::string registrationResponseChannel = "p2d/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isMessageFromPlatform =
      wolkabout::DeviceRegistrationProtocol::isMessageFromPlatform(registrationResponseChannel);

    // Then
    ASSERT_TRUE(isMessageFromPlatform);
}

TEST(DeviceRegistrationProtocol,
     Given_MessageToPlatform_When_MessageDirectionIsChecked_Then_MessageDirectionEqualsToPlatform)
{
    // Given
    const std::string registrationResponseChannel = "d2p/register_device/g/GATEWAY_KEY/d/DEVICE_KEY";

    // When
    const bool isMessageToPlatform =
      wolkabout::DeviceRegistrationProtocol::isMessageToPlatform(registrationResponseChannel);

    // Then
    ASSERT_TRUE(isMessageToPlatform);
}

TEST(DeviceRegistrationProtocol, VerifyDeviceTopics)
{
    std::vector<std::string> deviceTopics = wolkabout::DeviceRegistrationProtocol::getDeviceTopics();

    ASSERT_THAT(deviceTopics, ::testing::ElementsAre("d2p/register_device/d/#"));
}

TEST(DeviceRegistrationProtocol, VerifyPlatformTopics)
{
    std::vector<std::string> platformTopics = wolkabout::DeviceRegistrationProtocol::getPlatformTopics();

    ASSERT_THAT(platformTopics, ::testing::ElementsAre("p2d/register_device/g/#", "p2d/reregister_device/g/#"));
}
