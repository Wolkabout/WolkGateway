#include "OutboundMessageHandler.h"
#include "model/GatewayDevice.h"
#include "model/GatewayUpdateRequest.h"
#include "model/GatewayUpdateResponse.h"
#include "model/Message.h"
#include "model/SubdeviceManagement.h"
#include "model/SubdeviceRegistrationRequest.h"
#include "model/SubdeviceRegistrationResponse.h"
#include "protocol/json/JsonGatewaySubdeviceRegistrationProtocol.h"
#include "protocol/json/JsonRegistrationProtocol.h"
#include "repository/DeviceRepository.h"
#include "repository/SQLiteDeviceRepository.h"
#include "service/GatewayUpdateService.h"
#include "service/SubdeviceRegistrationService.h"

#include <gtest/gtest.h>
#include <cstdio>
#include <map>
#include <memory>
#include <string>
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
        protocol.reset(new wolkabout::JsonRegistrationProtocol());
        gatewayProtocol.reset(new wolkabout::JsonGatewaySubdeviceRegistrationProtocol());
        deviceRepository = std::unique_ptr<wolkabout::SQLiteDeviceRepository>(
          new wolkabout::SQLiteDeviceRepository(DEVICE_REPOSITORY_PATH));
        platformOutboundMessageHandler =
          std::unique_ptr<PlatformOutboundMessageHandler>(new PlatformOutboundMessageHandler());
        deviceOutboundMessageHandler =
          std::unique_ptr<DeviceOutboundMessageHandler>(new DeviceOutboundMessageHandler());
        deviceRegistrationService = std::unique_ptr<wolkabout::SubdeviceRegistrationService>(
          new wolkabout::SubdeviceRegistrationService(GATEWAY_KEY, *protocol, *gatewayProtocol, *deviceRepository,
                                                      *platformOutboundMessageHandler, *deviceOutboundMessageHandler));
        gatewayUpdateService = std::unique_ptr<wolkabout::GatewayUpdateService>(new wolkabout::GatewayUpdateService(
          GATEWAY_KEY, *protocol, *deviceRepository, *platformOutboundMessageHandler));
    }

    void TearDown() override { remove(DEVICE_REPOSITORY_PATH); }

    std::unique_ptr<wolkabout::RegistrationProtocol> protocol;
    std::unique_ptr<wolkabout::GatewaySubdeviceRegistrationProtocol> gatewayProtocol;
    std::unique_ptr<wolkabout::SQLiteDeviceRepository> deviceRepository;
    std::unique_ptr<PlatformOutboundMessageHandler> platformOutboundMessageHandler;
    std::unique_ptr<DeviceOutboundMessageHandler> deviceOutboundMessageHandler;
    std::unique_ptr<wolkabout::SubdeviceRegistrationService> deviceRegistrationService;
    std::unique_ptr<wolkabout::GatewayUpdateService> gatewayUpdateService;

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
       Given_ThatNoDeviceIsRegistered_When_GatewayRequestsUpdate_Then_UpdateRequestIsForwardedToPlatform)
{
    // Given
    // Intentionally left empty

    // When
    wolkabout::DeviceTemplate gatewayTemplate;
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);

    gatewayUpdateService->updateGateway(gateway);

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_ThatGatewayIsUpdatedAndManagesSubdevices_When_DeviceOtherThanGatewayRequestsRegistration_Then_RegistrationRequestIsForwardedToPlatform)
{
    // Given
    wolkabout::GatewayDevice gateway(GATEWAY_KEY, "", wolkabout::SubdeviceManagement::GATEWAY, true, true);
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
    wolkabout::GatewayDevice gateway(GATEWAY_KEY, "", wolkabout::SubdeviceManagement::GATEWAY, true, true);
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
    wolkabout::GatewayDevice gateway(GATEWAY_KEY, "", wolkabout::SubdeviceManagement::GATEWAY, true, true);
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
  Given_GatewayUpdateAwaitingPlatformResponse_When_GatewayIsSuccessfullyUpdated_Then_OnGatewayUpdatedCallbackIsInvoked)
{
    // Given
    bool isRegisteredDeviceGateway;
    gatewayUpdateService->onGatewayUpdated([&]() -> void { isRegisteredDeviceGateway = true; });

    wolkabout::GatewayDevice gateway(GATEWAY_KEY, "", wolkabout::SubdeviceManagement::GATEWAY, true, true);

    gatewayUpdateService->updateGateway(gateway);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    // When
    auto message = std::make_shared<wolkabout::Message>("{\"result\":\"OK\", \"description\": null}",
                                                        "p2d/update_gateway_response/g/GATEWAY_KEY");
    gatewayUpdateService->platformMessageReceived(message);

    // Then
    EXPECT_TRUE(isRegisteredDeviceGateway);
}

TEST_F(
  SubdeviceRegistrationService,
  Given_DeviceRegistrationAwaitingPlatformResponse_When_DeviceIsSuccessfullyRegistered_Then_OnDeviceRegisteredCallbackIsInvoked)
{
    // Given
    std::string registeredDeviceKey;
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
    auto channel = std::string("p2d/subdevice_registration_response/g/") + GATEWAY_KEY;
    auto payload = R"({"payload":{"deviceKey":")" + deviceKey + R"(}, "result":"OK", "description":""})";
    auto deviceRegistrationResponseMessage = std::make_shared<wolkabout::Message>(channel, payload);
    deviceRegistrationService->platformMessageReceived(deviceRegistrationResponseMessage);

    // Then
    ASSERT_TRUE(deviceKey == registeredDeviceKey);
}

TEST_F(
  SubdeviceRegistrationService,
  Given_GatewayUpdateAwaitingPlatformResponse_When_SuccessfulGatewayUpdateResonseIsReceived_Then_UpdatedGatewayIsSavedToDeviceRepository)
{
    // Given
    wolkabout::GatewayDevice gateway(GATEWAY_KEY, "", wolkabout::SubdeviceManagement::GATEWAY, true, true);

    gatewayUpdateService->updateGateway(gateway);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    // When
    auto message = std::make_shared<wolkabout::Message>("{\"result\":\"OK\", \"description\": null}",
                                                        "p2d/update_gateway_response/g/GATEWAY_KEY");
    gatewayUpdateService->platformMessageReceived(message);

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
    auto channel = std::string("p2d/subdevice_registration_response/g/") + GATEWAY_KEY;
    auto payload = R"({"payload":{"deviceKey":")" + deviceKey + R"(}, "result":"OK", "description":""})";
    auto deviceRegistrationResponseMessage = std::make_shared<wolkabout::Message>(channel, payload);
    deviceRegistrationService->platformMessageReceived(deviceRegistrationResponseMessage);

    // Then
    ASSERT_NE(nullptr, deviceRepository->findByDeviceKey(deviceKey));
}

TEST_F(
  SubdeviceRegistrationService,
  Given_ThatGatewayIsNotUpdatedAndListOfSubdeviceRegistrationRequestsAndGatewayUpdateRequest_When_GatewayIsRegistered_Then_PostponedSubdeviceRegistrationRequestsAreForwardedToPlatform)
{
    // Given
    wolkabout::GatewayDevice gateway(GATEWAY_KEY, "", wolkabout::SubdeviceManagement::GATEWAY, true, true);

    gatewayUpdateService->onGatewayUpdated([&]() -> void { deviceRegistrationService->registerPostponedDevices(); });

    gatewayUpdateService->updateGateway(gateway);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    const std::string deviceKey("deviceKey");
    wolkabout::DeviceTemplate deviceTemplate;
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);

    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    // When
    auto message = std::make_shared<wolkabout::Message>("{\"result\":\"OK\", \"description\": null}",
                                                        "p2d/update_gateway_response/g/GATEWAY_KEY");
    gatewayUpdateService->platformMessageReceived(message);

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
    auto channel = std::string("p2d/subdevice_registration_response/g/") + GATEWAY_KEY;
    auto payload = R"({"payload":{"deviceKey":")" + deviceKey + R"(}, "result":"OK", "description":""})";
    auto deviceRegistrationResponseMessage = std::make_shared<wolkabout::Message>(channel, payload);
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
    auto channel = std::string("p2d/subdevice_registration_response/g/") + GATEWAY_KEY;
    auto payload =
      R"({"payload":{"deviceKey":")" + deviceKey + R"(}, "result":"ERROR_VALIDATION_ERROR", "description":""})";
    auto deviceRegistrationResponseMessage = std::make_shared<wolkabout::Message>(channel, payload);

    deviceRegistrationService->platformMessageReceived(deviceRegistrationResponseMessage);

    // Then
    ASSERT_EQ(1, deviceOutboundMessageHandler->getMessages().size());
}
