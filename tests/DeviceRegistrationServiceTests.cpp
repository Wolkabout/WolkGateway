#include "OutboundMessageHandler.h"
#include "model/GatewayDevice.h"
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
        protocol = std::unique_ptr<wolkabout::GatewayDeviceRegistrationProtocol>(
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

    std::unique_ptr<wolkabout::GatewayDeviceRegistrationProtocol> protocol;
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
    wolkabout::DeviceTemplate deviceTemplate("Template name", "Template description", "JsonProtocol", "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);

    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);
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
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest gatewayRegistrationRequest("Gateway name", GATEWAY_KEY, gatewayTemplate);

    std::shared_ptr<wolkabout::Message> gatewayRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, GATEWAY_KEY, gatewayRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(gatewayRegistrationRequestMessage);

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_ThatGatewayIsRegistered_When_DeviceOtherThanGatewayRequestsRegistration_Then_RegistrationRequestIsForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    // When
    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate("Template name", "Template description", "JsonProtocol", "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);

    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_RegisteredDevice_When_AlreadyRegisteredDeviceRequestsRegistration_Then_RegistrationRequestIsNotForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate("Template name", "Template description", "JsonProtocol", "DFUProtocol");
    wolkabout::DetailedDevice device("Device name", deviceKey, deviceTemplate);

    deviceRepository->save(device);

    // When
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_ThatDeviceIsRegistered_When_AlreadyRegisteredDeviceRequestsRegistrationWithDifferentTemplate_Then_RegistrationRequestIsForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate("Template name", "Template description", "JsonProtocol", "DFUProtocol");
    wolkabout::DetailedDevice device("Device name", deviceKey, deviceTemplate);

    deviceRepository->save(device);

    // When
    deviceTemplate.addSensor(
      wolkabout::SensorTemplate("Sensor name", "ref", "TEXT", "", wolkabout::DataType::STRING, 1, "", {}, 0, 1));
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_GatewayRegisteredWithJsonDataProtocol_When_DeviceWithProtocolOtherThanJsonRequestsRegistration_Then_RegistrationRequestNotIsForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    // When
    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate("Template name", "Template description", "JsonSingleProtocol",
                                             "DFUProtocol");
    wolkabout::DetailedDevice device("Device name", deviceKey, deviceTemplate);

    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // Then
    ASSERT_TRUE(platformOutboundMessageHandler->getMessages().empty());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_GatewayRegisteredWithJsonDataProtocol_When_DeviceWithProtocolJsonRequestsRegistration_Then_RegistrationRequestIsForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    // When
    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate("Template name", "Template description", "JsonProtocol", "DFUProtocol");
    wolkabout::DetailedDevice device("Device name", deviceKey, deviceTemplate);

    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);
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
    deviceRegistrationService->onDeviceRegistered([&](const std::string& deviceKey, bool isGateway) -> void {
        registeredDeviceKey = deviceKey;
        isRegisteredDeviceGateway = isGateway;
    });

    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest gatewayRegistrationRequest("Gateway name", GATEWAY_KEY, gatewayTemplate);

    std::shared_ptr<wolkabout::Message> gatewayRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, GATEWAY_KEY, gatewayRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(gatewayRegistrationRequestMessage);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    // When
    wolkabout::SubdeviceRegistrationResponse gatewayRegistrationResponse(
      wolkabout::SubdeviceRegistrationResponse::Result::OK);
    std::shared_ptr<wolkabout::Message> gatewayRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, GATEWAY_KEY, gatewayRegistrationResponse);
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
    deviceRegistrationService->onDeviceRegistered([&](const std::string& deviceKey, bool isGateway) -> void {
        registeredDeviceKey = deviceKey;
        isRegisteredDeviceGateway = isGateway;
    });

    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate("Template name", "Template description", "JsonProtocol", "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);

    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // When
    wolkabout::SubdeviceRegistrationResponse deviceRegistrationResponse(
      wolkabout::SubdeviceRegistrationResponse::Result::OK);
    std::shared_ptr<wolkabout::Message> deviceRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationResponse);
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
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest gatewayRegistrationRequest("Gateway name", GATEWAY_KEY, gatewayTemplate);

    std::shared_ptr<wolkabout::Message> gatewayRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, GATEWAY_KEY, gatewayRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(gatewayRegistrationRequestMessage);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    // When
    wolkabout::SubdeviceRegistrationResponse gatewayRegistrationResponse(
      wolkabout::SubdeviceRegistrationResponse::Result::OK);
    std::shared_ptr<wolkabout::Message> gatewayRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, GATEWAY_KEY, gatewayRegistrationResponse);
    deviceRegistrationService->platformMessageReceived(gatewayRegistrationResponseMessage);

    // Then
    ASSERT_NE(nullptr, deviceRepository->findByDeviceKey(GATEWAY_KEY));
}

TEST_F(
  SubdeviceRegistrationService,
  Given_DeviceRegistrationAwaitingPlatformResponse_When_DeviceIsSuccessfullyRegistered_Then_RegisteredDeviceIsSavedToDeviceRepository)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate("Template name", "Template description", "JsonProtocol", "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);

    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // When
    wolkabout::SubdeviceRegistrationResponse deviceRegistrationResponse(
      wolkabout::SubdeviceRegistrationResponse::Result::OK);
    std::shared_ptr<wolkabout::Message> deviceRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationResponse);
    deviceRegistrationService->platformMessageReceived(deviceRegistrationResponseMessage);

    // Then
    ASSERT_NE(nullptr, deviceRepository->findByDeviceKey(deviceKey));
}

TEST_F(
  SubdeviceRegistrationService,
  Given_ThatGatewayIsNotRegisteredAndListOfSubdeviceRegistrationRequestsAndGatewayRegistrationRequest_When_GatewayIsRegistered_Then_PostponedSubdeviceRegistrationRequestsAreForwardedToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest gatewayRegistrationRequest("Gateway name", GATEWAY_KEY, gatewayTemplate);

    std::shared_ptr<wolkabout::Message> gatewayRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, GATEWAY_KEY, gatewayRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(gatewayRegistrationRequestMessage);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    const std::string deviceKey("deviceKey");
    wolkabout::DeviceTemplate deviceTemplate("Device manifest name", "Device manifest description", "JsonProtocol",
                                             "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);

    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);
    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());

    // When
    wolkabout::SubdeviceRegistrationResponse gatewayRegistrationResponse(
      wolkabout::SubdeviceRegistrationResponse::Result::OK);
    std::shared_ptr<wolkabout::Message> gatewayRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, GATEWAY_KEY, gatewayRegistrationResponse);
    deviceRegistrationService->platformMessageReceived(gatewayRegistrationResponseMessage);

    // Then
    ASSERT_EQ(2, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_RegisteredGatewayAndChildDevice_When_DeviceReregistrationRequestIsReceived_Then_DeviceReregistrationRequestIsForwardedToChildDevice)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    wolkabout::DeviceTemplate deviceTemplate("Device manifest name", "Device manifest description", "JsonProtocol",
                                             "DFUProtocol");
    wolkabout::DetailedDevice device("Child device", "child_device_key", deviceTemplate);
    deviceRepository->save(device);

    // When
    std::shared_ptr<wolkabout::Message> reregisterAllDevicesRequestMessage =
      protocol->makeDeviceReregistrationRequestForGateway(GATEWAY_KEY);
    deviceRegistrationService->platformMessageReceived(reregisterAllDevicesRequestMessage);

    // Then
    ASSERT_EQ(1, deviceOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_RegisteredGatewayAndChildDevice_When_DeviceReregistrationRequestIsReceived_Then_DeviceReregistrationRequestAcknowledgmentIsSentToPlatform)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    wolkabout::DeviceTemplate deviceTemplate("Device manifest name", "Device manifest description", "JsonProtocol",
                                             "DFUProtocol");
    wolkabout::DetailedDevice device("Child device", "child_device_key", deviceTemplate);
    deviceRepository->save(device);

    // When
    std::shared_ptr<wolkabout::Message> reregisterAllDevicesRequestMessage =
      protocol->makeDeviceReregistrationRequestForGateway(GATEWAY_KEY);
    deviceRegistrationService->platformMessageReceived(reregisterAllDevicesRequestMessage);

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_SingleRegisteredChildDevice_When_DevicesOtherThanChildDeviceAreDeleted_Then_NoDeletionRequestIsSentToPlatform)
{
    // Given
    const std::string childDeviceKey = "child_device_key";

    wolkabout::DeviceTemplate deviceTemplate("Device manifest name", "Device manifest description", "JsonProtocol",
                                             "DFUProtocol");
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

    wolkabout::DeviceTemplate deviceTemplate("Device manifest name", "Device manifest description", "JsonProtocol",
                                             "DFUProtocol");
    wolkabout::DetailedDevice device("Child device", childDeviceKey, deviceTemplate);
    deviceRepository->save(device);

    // When
    deviceRegistrationService->deleteDevicesOtherThan({});

    // Then
    ASSERT_EQ(1, platformOutboundMessageHandler->getMessages().size());
    ASSERT_TRUE(protocol->isDeviceDeletionRequest(*platformOutboundMessageHandler->getMessages().front()));
}

TEST_F(SubdeviceRegistrationService,
       Given_SingleRegisteredChildDevice_When_ChildDeviceisDeleted_Then_ChildDeviceIsDeletedFromDeviceRepository)
{
    // Given
    const std::string childDeviceKey = "child_device_key";

    wolkabout::DeviceTemplate deviceTemplate("Device manifest name", "Device manifest description", "JsonProtocol",
                                             "DFUProtocol");
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
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate("Template name", "Template description", "JsonProtocol", "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);

    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // When
    wolkabout::SubdeviceRegistrationResponse deviceRegistrationResponse(
      wolkabout::SubdeviceRegistrationResponse::Result::OK);
    std::shared_ptr<wolkabout::Message> deviceRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationResponse);
    deviceRegistrationService->platformMessageReceived(deviceRegistrationResponseMessage);

    // Then
    ASSERT_EQ(1, deviceOutboundMessageHandler->getMessages().size());
}

TEST_F(
  SubdeviceRegistrationService,
  Given_DeviceRegistrationAwaitingPlatformResponse_When_DeviceIsNotSuccessfullyRegistered_Then_ResponseIsForwardedToDevice)
{
    // Given
    wolkabout::DeviceTemplate gatewayTemplate("Gateway manifest name", "Gateway manifest description", "JsonProtocol",
                                              "DFUProtocol");
    wolkabout::DetailedDevice gateway("Gateway", GATEWAY_KEY, gatewayTemplate);
    deviceRepository->save(gateway);

    const std::string deviceKey("device_key");
    wolkabout::DeviceTemplate deviceTemplate("Template name", "Template description", "JsonProtocol", "DFUProtocol");
    wolkabout::SubdeviceRegistrationRequest deviceRegistrationRequest("Device name", deviceKey, deviceTemplate);
    std::shared_ptr<wolkabout::Message> deviceRegistrationRequestMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationRequest);

    deviceRegistrationService->deviceMessageReceived(deviceRegistrationRequestMessage);

    // When
    wolkabout::SubdeviceRegistrationResponse deviceRegistrationResponse(
      wolkabout::SubdeviceRegistrationResponse::Result::ERROR_READING_PAYLOAD);
    std::shared_ptr<wolkabout::Message> deviceRegistrationResponseMessage =
      protocol->makeMessage(GATEWAY_KEY, deviceKey, deviceRegistrationResponse);
    deviceRegistrationService->platformMessageReceived(deviceRegistrationResponseMessage);

    // Then
    ASSERT_EQ(1, deviceOutboundMessageHandler->getMessages().size());
}
