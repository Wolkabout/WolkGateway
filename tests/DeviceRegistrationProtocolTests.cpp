#include "model/Message.h"
#define private public
#define protected public
#include "protocol/json/JsonGatewaySubdeviceRegistrationProtocol.h"
#undef protected
#undef private

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
    const std::string registrationRequestChannel = "d2p/register_subdevice_request/d/DEVICE_KEY/";

    // When
    const std::string deviceKey = protocol->extractDeviceKeyFromChannel(registrationRequestChannel);

    // Then
    ASSERT_EQ("DEVICE_KEY", deviceKey);
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

TEST_F(JsonGatewaySubdeviceRegistrationProtocol, VerifyDeviceTopics)
{
    std::vector<std::string> deviceTopics = protocol->getInboundChannels();

    ASSERT_THAT(deviceTopics, ::testing::ElementsAre("d2p/register_subdevice_request/d/#"));
}
