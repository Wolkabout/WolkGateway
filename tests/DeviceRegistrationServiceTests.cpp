#include "OutboundMessageHandler.h"
#include "model/GatewayDevice.h"
#include "model/GatewayUpdateRequest.h"
#include "model/GatewayUpdateResponse.h"
#include "model/Message.h"
#include "model/SubdeviceRegistrationRequest.h"
#include "model/SubdeviceRegistrationResponse.h"
#include "protocol/json/JsonGatewaySubdeviceRegistrationProtocol.h"
#include "repository/DeviceRepository.h"
#include "repository/SQLiteDeviceRepository.h"
#include "service/SubdeviceRegistrationService.h"

#include <gtest/gtest.h>
#include <cstdio>
#include <memory>
#include <vector>

namespace
{
class PlatformOutboundMessageHandler : public wolkabout::OutboundMessageHandler
{
public:
    void addMessage(std::shared_ptr<wolkabout::Message> message) override { m_messages.push_back(message); }

    const std::vector<std::shared_ptr<wolkabout::Message>>& getMessages() const { return m_messages; }

private:
    std::vector<std::shared_ptr<wolkabout::Message>> m_messages;
};

class DeviceOutboundMessageHandler : public wolkabout::OutboundMessageHandler
{
public:
    void addMessage(std::shared_ptr<wolkabout::Message> message) override { m_messages.push_back(message); }

    const std::vector<std::shared_ptr<wolkabout::Message>>& getMessages() const { return m_messages; }

private:
    std::vector<std::shared_ptr<wolkabout::Message>> m_messages;
};

class SubdeviceRegistrationService : public ::testing::Test
{
public:
    void SetUp() override
    {
        protocol = std::unique_ptr<wolkabout::GatewaySubdeviceRegistrationProtocol>(
          new wolkabout::JsonGatewaySubdeviceRegistrationProtocol());
        deviceRepository = std::unique_ptr<wolkabout::SQLiteDeviceRepository>(
          new wolkabout::SQLiteDeviceRepository(DEVICE_REPOSITORY_PATH));
        platformOutboundMessageHandler =
          std::unique_ptr<PlatformOutboundMessageHandler>(new PlatformOutboundMessageHandler());
        deviceOutboundMessageHandler =
          std::unique_ptr<DeviceOutboundMessageHandler>(new DeviceOutboundMessageHandler());
        deviceRegistrationService =
          std::unique_ptr<wolkabout::SubdeviceRegistrationService>(new wolkabout::SubdeviceRegistrationService(
            GATEWAY_KEY, *protocol, *deviceRepository, *platformOutboundMessageHandler, *deviceOutboundMessageHandler));
    }

    void TearDown() override { remove(DEVICE_REPOSITORY_PATH); }

    std::unique_ptr<wolkabout::GatewaySubdeviceRegistrationProtocol> protocol;
    std::unique_ptr<wolkabout::SQLiteDeviceRepository> deviceRepository;
    std::unique_ptr<PlatformOutboundMessageHandler> platformOutboundMessageHandler;
    std::unique_ptr<DeviceOutboundMessageHandler> deviceOutboundMessageHandler;
    std::unique_ptr<wolkabout::SubdeviceRegistrationService> deviceRegistrationService;

    static constexpr const char* DEVICE_REPOSITORY_PATH = "testsDeviceRepository.db";
    static constexpr const char* GATEWAY_KEY = "gateway_key";
};
}    // namespace

TEST_F(
  SubdeviceRegistrationService,
  Given_ThatNoDeviceIsRegistered_When_DeviceOtherThanGatewayRequestsRegistration_Then_RegistrationRequestIsNotForwardedToPlatform)
{
    // Given
    // Intentionally left empty

    // When
    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);

    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(SubdeviceRegistrationService,
       Given_ThatNoDeviceIsRegistered_When_GatewayRequestsRegistration_Then_RegistrationRequestIsForwardedToPlatform)
{
    // Given
    // Intentionally left empty

    // When
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::GatewayUpdateRequest gatewayRegistrationRequest("Gateway name", GATEWAY_KEY, gatewayTemplate);

    std::shared_ptr<wolkabout::Message> gatewayRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, gatewayRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(gatewayRegistrationRequestMessage);

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_ThatGatewayIsRegistered_When_DeviceOtherThanGatewayRequestsRegistration_Then_RegistrationRequestIsForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    // When
    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);

    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_RegisteredDevice_When_AlreadyRegisteredDeviceRequestsRegistration_Then_RegistrationRequestIsNotForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::DetailedDevice device("Device name", deviceKey, deviceTemplate);

    deviceRepository->save(device);

    // When
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_ThatDeviceIsRegistered_When_AlreadyRegisteredDeviceRequestsRegistrationWithDifferentTemplate_Then_RegistrationRequestIsForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::DetailedDevice device("Device name", deviceKey, deviceTemplate);

    deviceRepository->save(device);

    // When
    deviceTemplate.addSensor(wolkabout::SensorTemplate("Sensor name", "ref", wolkabout::DataType::STRING, "", {}, {}));
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_GatewayRegisteredWithJsonDataProtocol_When_DeviceWithProtocolOtherThanJsonRequestsRegistration_Then_RegistrationRequestNotIsForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    // When
    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::DetailedDevice device("Device name", deviceKey, deviceTemplate);

    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_GatewayRegisteredWithJsonDataProtocol_When_DeviceWithProtocolJsonRequestsRegistration_Then_RegistrationRequestIsForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    // When
    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate;

    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_GatewayRegistrationAwaitingPlatformResponse_When_DeviceIsSuccessfullyRegistered_Then_OnDeviceRegisteredCallbackIsInvoked)
{
    // Given
    std::string registeredDeviceKey;
    bool isRegisteredDeviceGateway;
    deviceRegistrationService->onDeviceRegistered(
      [&](const std::string& deviceKey) -> void { registeredDeviceKey = deviceKey; });

    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::SubdeviceRegistrationRequest gatewayRegistrationRequest("Gateway name", GATEWAY_KEY, gatewayTemplate);

    std::shared_ptr<wolkabout::Message> gatewayRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, gatewayRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(gatewayRegistrationRequestMessage);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    // When
    wolkabout::SubdeviceRegistrationResponse gatewayRegistrationResponse(
      GATEWAY_KEY, wolkabout::SubdeviceRegistrationResponse::Result::OK, "");
    std::shared_ptr<wolkabout::Message> gatewayRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, gatewayRegistrationResponse);
    deviceRegistrationService->platformMessageReceived(gatewayRegistrationResponseMessage);

    // Then
    ASSERT_TRUE(GATEWAY_KEY == registeredDeviceKey);
    EXPECT_TRUE(isRegisteredDeviceGateway);
}

TEST_F(
  SubdeviceRegistrationService,
  Given_DeviceRegistrationAwaitingPlatformResponse_When_DeviceIsSuccessfullyRegistered_Then_OnDeviceRegisteredCallbackIsInvoked)
{
    // Given
    std::string registeredDeviceKey;
    bool isRegisteredDeviceGateway;
    deviceRegistrationService->onDeviceRegistered(
      [&](const std::string& deviceKey) -> void { registeredDeviceKey = deviceKey; });

    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);

    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // When
    wolkabout::SubdeviceRegistrationResponse deviceRegistrationResponse(
      deviceKey, wolkabout::SubdeviceRegistrationResponse::Result::OK, "");
    std::shared_ptr<wolkabout::Message> deviceRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationResponse);
    deviceRegistrationService->platformMessageReceived(deviceRegistrationResponseMessage);

    // Then
    ASSERT_TRUE(deviceKey == registeredDeviceKey);
    EXPECT_FALSE(isRegisteredDeviceGateway);
}

TEST_F(
  SubdeviceRegistrationService,
  Given_GatewayRegistrationAwaitingPlatformResponse_When_SuccessfulGatewayRegistrationResonseIsReceived_Then_RegisteredGatewayIsSavedToDeviceRepository)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::SubdeviceRegistrationRequest gatewayRegistrationRequest("Gateway name", GATEWAY_KEY, gatewayTemplate);

    std::shared_ptr<wolkabout::Message> gatewayRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, gatewayRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(gatewayRegistrationRequestMessage);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    // When
    wolkabout::GatewayUpdateResponse gatewayRegistrationResponse(wolkabout::GatewayUpdateResponse::Result::OK, "");
    auto message =
      std::make_shared<wolkabout::Message>("{\"result\":\"OK\"}", "p2d/gateway_update_response/g/GATEWAY_KEY");
    deviceRegistrationService->platformMessageReceived(message);

    // Then
    ASSERT_NE(nullptr, deviceRepository->findByDeviceKey(GATEWAY_KEY));
}

TEST_F(
  SubdeviceRegistrationService,
  Given_DeviceRegistrationAwaitingPlatformResponse_When_DeviceIsSuccessfullyRegistered_Then_RegisteredDeviceIsSavedToDeviceRepository)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);

    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // When
    wolkabout::SubdeviceRegistrationResponse deviceRegistrationResponse(
      deviceKey, wolkabout::SubdeviceRegistrationResponse::Result::OK);
    std::shared_ptr<wolkabout::Message> deviceRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationResponse);
    deviceRegistrationService->platformMessageReceived(deviceRegistrationResponseMessage);

    // Then
    ASSERT_NE(nullptr, deviceRepository->findByDeviceKey(deviceKey));
}

TEST_F(
  SubdeviceRegistrationService,
  Given_ThatGatewayIsNotRegisteredAndListOfSubdeviceRegistrationRequestsAndGatewayRegistrationRequest_When_GatewayIsRegistered_Then_PostponedSubdeviceRegistrationRequestsAreForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::SubdeviceRegistrationRequest gatewayRegistrationRequest("Gateway name", GATEWAY_KEY, gatewayTemplate);

    std::shared_ptr<wolkabout::Message> gatewayRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, gatewayRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(gatewayRegistrationRequestMessage);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    const std::string deviceKey("deviceKey");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);

    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    // When
    auto message = std::make_shared<wolkabout::Message>("", "p2d/gateway_update_response/g/GATEWAY_KEY");
    deviceRegistrationService->platformMessageReceived(message);

    // Then
    ASSERT_EQ(2, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_SingleRegisteredChildDevice_When_DevicesOtherThanChildDeviceAreDeleted_Then_NoDeletionRequestIsSentToPlatform)
{
    // Given
    const std::string childDeviceKey = "child_device_key";

    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::DetailedDevice device("Child device", childDeviceKey, deviceTemplate);
    deviceRepository->save(device);

    // When
    deviceRegistrationService->deleteDevicesOtherThan({childDeviceKey});

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(SubdeviceRegistrationService,
       Given_SingleRegisteredChildDevice_When_ChildDeviceisDeleted_Then_DeletionRequestIsSentToPlatform)
{
    // Given
    const std::string childDeviceKey = "child_device_key";

    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::DetailedDevice device("Child device", childDeviceKey, deviceTemplate);
    deviceRepository->save(device);

    // When
    deviceRegistrationService->deleteDevicesOtherThan({});

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
    ASSERT_TRUE(protocol->isSubdeviceDeletionRequest(*platformOutboundMessageHandler->getMessages().front()));
}

TEST_F(SubdeviceRegistrationService,
       Given_SingleRegisteredChildDevice_When_ChildDeviceisDeleted_Then_ChildDeviceIsDeletedFromDeviceRepository)
{
    // Given
    const std::string childDeviceKey = "child_device_key";

    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::DetailedDevice device("Child device", childDeviceKey, deviceTemplate);
    deviceRepository->save(device);

    // When
    deviceRegistrationService->deleteDevicesOtherThan({});

    // Then
    ASSERT_FALSE(deviceRepository->containsDeviceWithKey(childDeviceKey));
}

TEST_F(
  SubdeviceRegistrationService,
  Given_DeviceRegistrationAwaitingPlatformResponse_When_DeviceIsSuccessfullyRegistered_Then_ResponseIsForwardedToDevice)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);

    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // When
    wolkabout::SubdeviceRegistrationResponse deviceRegistrationResponse(
      deviceKey, wolkabout::SubdeviceRegistrationResponse::Result::OK, "");
    std::shared_ptr<wolkabout::Message> deviceRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationResponse);
    deviceRegistrationService->platformMessageReceived(deviceRegistrationResponseMessage);

    // Then
    ASSERT_EQ(1, deviceOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_DeviceRegistrationAwaitingPlatformResponse_When_DeviceIsNotSuccessfullyRegistered_Then_ResponseIsForwardedToDevice)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);

    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // When
    wolkabout::SubdeviceRegistrationResponse deviceRegistrationResponse(
      deviceKey, wolkabout::SubdeviceRegistrationResponse::Result::ERROR_VALIDATION_ERROR, "");
    std::shared_ptr<wolkabout::Message> deviceRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationResponse);
    deviceRegistrationService->platformMessageReceived(deviceRegistrationResponseMessage);

    // Then
    ASSERT_EQ(1, deviceOutboundMessageHandler->getMessages().size());
}
