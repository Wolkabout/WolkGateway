#define private public
#define protected public
#include "Wolk.h"
#include "WolkBuilder.h"
#undef protected
#undef private
#include "model/GatewayDevice.h"
#include "model/SubdeviceManagement.h"

#include <gtest/gtest.h>

namespace
{
class WolkBuilder : public ::testing::Test
{
public:
    void SetUp() override {}

    void TearDown() override {}

    std::unique_ptr<wolkabout::Wolk> wolk;

    static constexpr const char* GATEWAY_KEY = "gateway_key";
    static constexpr const char* GATEWAY_PASSWORD = "gateway_password";
};
}    // namespace

TEST_F(WolkBuilder, GivenGatewayManagesSubdevices_When_ConstructingWolkInstance_Then_RegistrationServiceIsSetup)
{
    // Given
    wolkabout::GatewayDevice device(GATEWAY_KEY, GATEWAY_PASSWORD, wolkabout::SubdeviceManagement::GATEWAY);
    wolkabout::WolkBuilder builder = wolkabout::Wolk::newBuilder(device);

    // When
    wolk = builder.build();

    // Then
    ASSERT_NE(nullptr, wolk->m_subdeviceRegistrationService);
}

TEST_F(WolkBuilder, GivenPlatformManagesSubdevices_When_ConstructingWolkInstance_Then_RegistrationServiceIsNotSetup)
{
    // Given
    wolkabout::GatewayDevice device(GATEWAY_KEY, GATEWAY_PASSWORD, wolkabout::SubdeviceManagement::PLATFORM);
    wolkabout::WolkBuilder builder = wolkabout::Wolk::newBuilder(device);

    // When
    wolk = builder.build();

    // Then
    ASSERT_EQ(nullptr, wolk->m_subdeviceRegistrationService);
}
