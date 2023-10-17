/**
 * Copyright 2022 Wolkabout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <any>
#include <sstream>

#define private public
#define protected public
#include "gateway/service/devices/DevicesService.h"
#undef private
#undef protected

#include "core/utility/Logger.h"
#include "tests/mocks/DeviceRepositoryMock.h"
#include "tests/mocks/ExistingDeviceRepositoryMock.h"
#include "tests/mocks/GatewayRegistrationProtocolMock.h"
#include "tests/mocks/OutboundMessageHandlerMock.h"
#include "tests/mocks/OutboundRetryMessageHandlerMock.h"
#include "tests/mocks/RegistrationProtocolMock.h"

#include <gtest/gtest.h>

using namespace wolkabout;
using namespace wolkabout::gateway;
using namespace ::testing;

class DevicesServiceTests : public Test
{
public:
    static void SetUpTestCase() { Logger::init(LogLevel::TRACE, Logger::Type::CONSOLE); }

    void SetUp() override
    {
        registrationProtocolMock = std::make_shared<StrictMock<RegistrationProtocolMock>>();
        platformOutboundMessageHandlerMock = std::make_shared<StrictMock<OutboundMessageHandlerMock>>();
        platformOutboundRetryMessageHandlerMock =
          std::make_shared<StrictMock<OutboundRetryMessageHandlerMock>>(*platformOutboundMessageHandlerMock);
        gatewayRegistrationProtocolMock = std::make_shared<StrictMock<GatewayRegistrationProtocolMock>>();
        localOutboundMessageHandlerMock = std::make_shared<StrictMock<OutboundMessageHandlerMock>>();
        deviceRepositoryMock = std::make_shared<StrictMock<DeviceRepositoryMock>>();
        existingDevicesRepositoryMock = std::make_shared<ExistingDevicesRepositoryMock>();
        service = std::unique_ptr<DevicesService>{
          new DevicesService{GATEWAY_KEY, *registrationProtocolMock, *platformOutboundMessageHandlerMock,
                             *platformOutboundRetryMessageHandlerMock, gatewayRegistrationProtocolMock,
                             localOutboundMessageHandlerMock, deviceRepositoryMock, existingDevicesRepositoryMock}};
    }

    std::unique_ptr<DevicesService> service;

    const std::string GATEWAY_KEY = "TEST_GATEWAY";

    const std::string DEVICE_KEY = "TEST_DEVICE";

    std::shared_ptr<RegistrationProtocolMock> registrationProtocolMock;

    std::shared_ptr<OutboundMessageHandlerMock> platformOutboundMessageHandlerMock;

    std::shared_ptr<OutboundRetryMessageHandlerMock> platformOutboundRetryMessageHandlerMock;

    std::shared_ptr<GatewayRegistrationProtocolMock> gatewayRegistrationProtocolMock;

    std::shared_ptr<OutboundMessageHandlerMock> localOutboundMessageHandlerMock;

    std::shared_ptr<DeviceRepositoryMock> deviceRepositoryMock;

    std::shared_ptr<ExistingDevicesRepositoryMock> existingDevicesRepositoryMock;

    std::mutex mutex;
    std::condition_variable conditionVariable;
};

TEST_F(DevicesServiceTests, GetProtocolLocalCommunicationDisabled)
{
    service->m_localProtocol = nullptr;
    EXPECT_THROW(service->getProtocol(), std::runtime_error);
}

TEST_F(DevicesServiceTests, GetProtocol)
{
    EXPECT_EQ(&(service->getProtocol()), gatewayRegistrationProtocolMock.get());
}

TEST_F(DevicesServiceTests, MessageTypes)
{
    auto types = std::vector<MessageType>{};
    ASSERT_NO_FATAL_FAILURE(types = service->getMessageTypes());
    EXPECT_EQ(types.size(), 2);
    EXPECT_FALSE(std::find(types.cbegin(), types.cend(), MessageType::CHILDREN_SYNCHRONIZATION_RESPONSE) ==
                 types.cend());
    EXPECT_FALSE(std::find(types.cbegin(), types.cend(), MessageType::REGISTERED_DEVICES_RESPONSE) == types.cend());
}

TEST_F(DevicesServiceTests, DeviceExistsNoRepository)
{
    service->m_deviceRepository = nullptr;
    EXPECT_FALSE(service->deviceExists(DEVICE_KEY));
}

TEST_F(DevicesServiceTests, DeviceExistsRepository)
{
    EXPECT_CALL(*deviceRepositoryMock, containsDevice(DEVICE_KEY)).WillOnce(Return(true));
    EXPECT_TRUE(service->deviceExists(DEVICE_KEY));
}

TEST_F(DevicesServiceTests, HandleChildrenSynchronizationResponseWithCallback)
{
    // Create a callback that will be invoked
    auto responseMessage = std::unique_ptr<ChildrenSynchronizationResponseMessage>{
      new ChildrenSynchronizationResponseMessage{{"Child1", "Child2"}}};
    const auto address = responseMessage.get();
    std::atomic_bool called{false};
    service->m_childSyncRequests.push(std::make_shared<ChildrenSynchronizationRequestCallback>(
      [&](const std::shared_ptr<ChildrenSynchronizationResponseMessage>& message) {
          EXPECT_EQ(address, message.get());
          called = true;
          conditionVariable.notify_one();
      }));

    // Set up the repositories to be invoked
    EXPECT_CALL(*deviceRepositoryMock, save).Times(1);
    EXPECT_CALL(*existingDevicesRepositoryMock, getDeviceKeys).WillOnce(Return(std::vector<std::string>{"Child1"}));
    EXPECT_CALL(*existingDevicesRepositoryMock, addDeviceKey).Times(1);

    // Invoke the method
    ASSERT_NO_FATAL_FAILURE(service->handleChildrenSynchronizationResponse(std::move(responseMessage)));
    if (!called)
    {
        std::unique_lock<std::mutex> lock{mutex};
        conditionVariable.wait_for(lock, std::chrono::milliseconds{100});
    }
    EXPECT_TRUE(called);
}

TEST_F(DevicesServiceTests, HandlerRegisteredDevicesResponseWithCallback)
{
    // Create a callback that will be invoked
    std::atomic_bool called{false};
    auto responseMessage = std::unique_ptr<RegisteredDevicesResponseMessage>{new RegisteredDevicesResponseMessage{
      std::chrono::milliseconds{1234567890}, "Type1", {}, {{"Device1", "Id1", "Type1"}, {"Device2", "Id2", "Type1"}}}};
    const auto address = responseMessage.get();
    service->m_registeredDevicesRequests.emplace(
      RegisteredDevicesRequestParameters{responseMessage->getTimestampFrom(), responseMessage->getDeviceType(),
                                         responseMessage->getExternalId()},
      std::make_shared<RegisteredDevicesRequestCallback>(
        [&](const std::shared_ptr<RegisteredDevicesResponseMessage>& message) {
            EXPECT_EQ(address, message.get());
            called = true;
            conditionVariable.notify_one();
        }));

    // Set up the device repository to be invoked
    EXPECT_CALL(*deviceRepositoryMock, save).Times(1);

    // Invoke the method
    ASSERT_NO_FATAL_FAILURE(service->handleRegisteredDevicesResponse(std::move(responseMessage)));
    if (!called)
    {
        std::unique_lock<std::mutex> lock{mutex};
        conditionVariable.wait_for(lock, std::chrono::milliseconds{100});
    }
    EXPECT_TRUE(called);
}

TEST_F(DevicesServiceTests, ReceivedMessagesOneMessageOfUnknownType)
{
    // Set up the expected calls
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, messageReceived).Times(1);
    EXPECT_CALL(*registrationProtocolMock, getMessageType).WillOnce(Return(MessageType::UNKNOWN));

    // Invoke the service
    ASSERT_NO_FATAL_FAILURE(
      service->receiveMessages(std::vector<GatewaySubdeviceMessage>{GatewaySubdeviceMessage{{"", ""}}}));
}

TEST_F(DevicesServiceTests, ReceivedMessageTwoMessagesBothNull)
{
    // Set up the expected calls
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, messageReceived).Times(2);
    EXPECT_CALL(*registrationProtocolMock, getMessageType)
      .WillOnce(Return(MessageType::CHILDREN_SYNCHRONIZATION_RESPONSE))
      .WillOnce(Return(MessageType::REGISTERED_DEVICES_RESPONSE));
    EXPECT_CALL(*registrationProtocolMock, parseChildrenSynchronizationResponse).WillOnce(Return(ByMove(nullptr)));
    EXPECT_CALL(*registrationProtocolMock, parseRegisteredDevicesResponse).WillOnce(Return(ByMove(nullptr)));

    // Invoke the service
    ASSERT_NO_FATAL_FAILURE(service->receiveMessages(
      std::vector<GatewaySubdeviceMessage>{GatewaySubdeviceMessage{{"", ""}}, GatewaySubdeviceMessage{{"", ""}}}));
}

TEST_F(DevicesServiceTests, ReceivedMessageTwoMessagesBothCallTheActualMethods)
{
    // Set up the expected calls
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, messageReceived).Times(2);
    EXPECT_CALL(*registrationProtocolMock, getMessageType)
      .WillOnce(Return(MessageType::CHILDREN_SYNCHRONIZATION_RESPONSE))
      .WillOnce(Return(MessageType::REGISTERED_DEVICES_RESPONSE));
    EXPECT_CALL(*registrationProtocolMock, parseChildrenSynchronizationResponse)
      .WillOnce(Return(ByMove(
        std::unique_ptr<ChildrenSynchronizationResponseMessage>{new ChildrenSynchronizationResponseMessage{{"C1"}}})));
    EXPECT_CALL(*registrationProtocolMock, parseRegisteredDevicesResponse)
      .WillOnce(Return(ByMove(std::unique_ptr<RegisteredDevicesResponseMessage>{
        new RegisteredDevicesResponseMessage{std::chrono::milliseconds{1234567890},
                                             "Type1",
                                             {},
                                             {{"Device1", "Id1", "Type1"}, {"Device2", "Id2", "Type1"}}}})));
    EXPECT_CALL(*deviceRepositoryMock, save).Times(2);
    EXPECT_CALL(*existingDevicesRepositoryMock, getDeviceKeys).Times(1);
    EXPECT_CALL(*existingDevicesRepositoryMock, addDeviceKey).Times(1);

    // Invoke the service
    ASSERT_NO_FATAL_FAILURE(service->receiveMessages(
      std::vector<GatewaySubdeviceMessage>{GatewaySubdeviceMessage{{"", ""}}, GatewaySubdeviceMessage{{"", ""}}}));
}

TEST_F(DevicesServiceTests, RegisterChildDevicesEmptyDevicesVector)
{
    EXPECT_FALSE(service->registerChildDevices({}, {}));
}

TEST_F(DevicesServiceTests, RegisterChildDevicesEmptyDeviceName)
{
    EXPECT_FALSE(service->registerChildDevices({DeviceRegistrationData{"", "", "", {}, {}, {}}}, {}));
}

TEST_F(DevicesServiceTests, RegisterChildDevicesEmptyDeviceKey)
{
    EXPECT_FALSE(service->registerChildDevices({DeviceRegistrationData{"Device Name", "", "", {}, {}, {}}}, {}));
}

TEST_F(DevicesServiceTests, RegisterChildDevicesProtocolFailsToParse)
{
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const DeviceRegistrationMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));
    EXPECT_FALSE(
      service->registerChildDevices({DeviceRegistrationData{"Device Name", "Device Key", "", {}, {}, {}}}, {}));
}

TEST_F(DevicesServiceTests, RegisterChildDevicesProtocolParses)
{
    // registerChildDevices call
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const DeviceRegistrationMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*platformOutboundMessageHandlerMock, addMessage).Times(1);

    // sendOutChildrenSynchronizationRequest call
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const ChildrenSynchronizationRequestMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*registrationProtocolMock,
                getResponseChannelForMessage(MessageType::CHILDREN_SYNCHRONIZATION_REQUEST, _))
      .Times(1);
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, addMessage).Times(1);

    // Now call the service and dig for the callback
    std::atomic_bool called{false};
    ASSERT_TRUE(service->registerChildDevices(
      {DeviceRegistrationData{"Device One", "D1", "", {}, {}, {}},
       DeviceRegistrationData{"Device Two", "D2", "", {}, {}, {}}},
      [&](const std::vector<std::string>& success, const std::vector<std::string>& failed) {
          if (!(success.empty() || failed.empty()))
              called = true;
      }));
    ASSERT_FALSE(service->m_childSyncRequests.empty());
    ASSERT_TRUE(service->m_childSyncRequests.front()->getLambda());
    ASSERT_NO_FATAL_FAILURE(service->m_childSyncRequests.front()->getLambda()(
      std::make_shared<ChildrenSynchronizationResponseMessage>(std::vector<std::string>{"D1"})));
    EXPECT_TRUE(called);
}

TEST_F(DevicesServiceTests, RemoveChildDevicesEmptyVector)
{
    EXPECT_FALSE(service->removeChildDevices({}));
}

TEST_F(DevicesServiceTests, RemoveChildDevicesEmptyKeyInVector)
{
    EXPECT_FALSE(service->removeChildDevices({{}}));
}

TEST_F(DevicesServiceTests, RemoveChildDevicesProtocolFailsToParse)
{
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const DeviceRemovalMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));
    EXPECT_FALSE(service->removeChildDevices({"Test Device Key"}));
}

TEST_F(DevicesServiceTests, RemoveChildDevicesProtocolParses)
{
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const DeviceRemovalMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*platformOutboundMessageHandlerMock, addMessage).Times(1);
    EXPECT_TRUE(service->removeChildDevices({"Test Device Key"}));
}

TEST_F(DevicesServiceTests, SendOutChildrenSynchronizationRequestFailsToParse)
{
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const ChildrenSynchronizationRequestMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));
    ASSERT_FALSE(service->sendOutChildrenSynchronizationRequest({}));
}

TEST_F(DevicesServiceTests, SendOutChildrenSynchronizationRequestRetryCallbackCalledLambda)
{
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const ChildrenSynchronizationRequestMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*registrationProtocolMock,
                getResponseChannelForMessage(MessageType::CHILDREN_SYNCHRONIZATION_REQUEST, _))
      .Times(1);
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, addMessage)
      .WillOnce([&](const RetryMessageStruct& retryMessageStruct) { retryMessageStruct.onFail({}); });
    std::atomic_bool called{false};
    ASSERT_TRUE(service->sendOutChildrenSynchronizationRequest(std::make_shared<ChildrenSynchronizationRequestCallback>(
      [&](const std::shared_ptr<ChildrenSynchronizationResponseMessage>&) { called = true; },
      std::vector<std::string>{"Device 1", "Device 2"})));
    EXPECT_TRUE(called);
}

TEST_F(DevicesServiceTests, SendOutRegisteredDevicesRequestFailsToParse)
{
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const RegisteredDevicesRequestMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));
    ASSERT_FALSE(service->sendOutRegisteredDevicesRequest(
      RegisteredDevicesRequestParameters{std::chrono::milliseconds{1234567890}}, {}));
}

TEST_F(DevicesServiceTests, SendOutRegisteredDevicesRequestRetryCallback)
{
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const RegisteredDevicesRequestMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*registrationProtocolMock, getResponseChannelForMessage(MessageType::REGISTERED_DEVICES_REQUEST, _))
      .Times(1);
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, addMessage)
      .WillOnce([&](const RetryMessageStruct& retryMessageStruct) { retryMessageStruct.onFail({}); });
    EXPECT_TRUE(service->m_registeredDevicesRequests.empty());
    ASSERT_TRUE(service->sendOutRegisteredDevicesRequest(
      RegisteredDevicesRequestParameters{std::chrono::milliseconds{1234567890}}, {}));
    EXPECT_FALSE(service->m_registeredDevicesRequests.empty());
}

TEST_F(DevicesServiceTests, UpdateDeviceCacheNoDeviceRepository)
{
    service->m_deviceRepository = nullptr;
    ASSERT_NO_FATAL_FAILURE(service->updateDeviceCache());
}

TEST_F(DevicesServiceTests, UpdateDeviceCacheWithDevicesToDeleteFailsToDelete)
{
    // Update calls
    EXPECT_CALL(*deviceRepositoryMock, latestPlatformTimestamp).Times(1);
    EXPECT_CALL(*deviceRepositoryMock, getGatewayDevices)
      .WillOnce(Return(std::vector<StoredDeviceInformation>{
        StoredDeviceInformation{"Test Device Key", DeviceOwnership::Gateway, std::chrono::milliseconds{0}}}));
    EXPECT_CALL(*deviceRepositoryMock, remove).Times(0);
    EXPECT_CALL(*existingDevicesRepositoryMock, getDeviceKeys).WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const DeviceRemovalMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));

    // Send out calls
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const ChildrenSynchronizationRequestMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const RegisteredDevicesRequestMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));

    // Call the service
    ASSERT_NO_FATAL_FAILURE(service->updateDeviceCache());
}

TEST_F(DevicesServiceTests, UpdateDeviceCacheWithDevicesToDeleteSucceedsToDelete)
{
    // Update calls
    EXPECT_CALL(*deviceRepositoryMock, latestPlatformTimestamp).Times(1);
    EXPECT_CALL(*deviceRepositoryMock, getGatewayDevices)
      .WillOnce(Return(std::vector<StoredDeviceInformation>{
        StoredDeviceInformation{"Test Device Key", DeviceOwnership::Gateway, std::chrono::milliseconds{0}}}));
    EXPECT_CALL(*deviceRepositoryMock, remove).Times(1);
    EXPECT_CALL(*existingDevicesRepositoryMock, getDeviceKeys).WillOnce(Return(std::vector<std::string>{}));
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const DeviceRemovalMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*platformOutboundMessageHandlerMock, addMessage).Times(1);

    // Send out calls
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const ChildrenSynchronizationRequestMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const RegisteredDevicesRequestMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));

    // Call the service
    ASSERT_NO_FATAL_FAILURE(service->updateDeviceCache());
}

TEST_F(DevicesServiceTests, MessageReceivedNoLocalProtocol)
{
    service->m_localProtocol = nullptr;
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(nullptr));
}

TEST_F(DevicesServiceTests, MessageReceivedMessageIsNull)
{
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(nullptr));
}

TEST_F(DevicesServiceTests, MessageReceivedMessageIsUnknown)
{
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType).WillOnce(Return(MessageType::UNKNOWN));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(""));
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(DevicesServiceTests, MessageReceivedDeviceRegistrationFailsToParse)
{
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType).WillOnce(Return(MessageType::DEVICE_REGISTRATION));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(DEVICE_KEY));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, parseDeviceRegistrationMessage).WillOnce(Return(ByMove(nullptr)));
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(DevicesServiceTests, MessageReceivedDeviceRegistrationRegistersDevicesButFailsToParseLocalMessage)
{
    // Handle calls
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType).WillOnce(Return(MessageType::DEVICE_REGISTRATION));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(DEVICE_KEY));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, parseDeviceRegistrationMessage)
      .WillOnce(Return(ByMove(std::unique_ptr<DeviceRegistrationMessage>{
        new DeviceRegistrationMessage{{DeviceRegistrationData{"Device Name 1", "D1", "", {}, {}, {}}}}})));

    // registerChildDevices call
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const DeviceRegistrationMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*platformOutboundMessageHandlerMock, addMessage).Times(1);

    // sendOutChildrenSynchronizationRequest call
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const ChildrenSynchronizationRequestMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*registrationProtocolMock,
                getResponseChannelForMessage(MessageType::CHILDREN_SYNCHRONIZATION_REQUEST, _))
      .Times(1);
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, addMessage).Times(1);

    // Callback
    EXPECT_CALL(*gatewayRegistrationProtocolMock, makeOutboundMessage(_, A<const DeviceRegistrationResponseMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));

    // Call the service and handle the callback
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
    ASSERT_FALSE(service->m_childSyncRequests.empty());
    ASSERT_TRUE(service->m_childSyncRequests.front()->getLambda());
    ASSERT_NO_FATAL_FAILURE(service->m_childSyncRequests.front()->getLambda()(
      std::make_shared<ChildrenSynchronizationResponseMessage>(std::vector<std::string>{"D1"})));
}

TEST_F(DevicesServiceTests, MessageReceivedDeviceRegistrationRegistersDevices)
{
    // Handle calls
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType).WillOnce(Return(MessageType::DEVICE_REGISTRATION));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(DEVICE_KEY));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, parseDeviceRegistrationMessage)
      .WillOnce(Return(ByMove(std::unique_ptr<DeviceRegistrationMessage>{
        new DeviceRegistrationMessage{{DeviceRegistrationData{"Device Name 1", "D1", "", {}, {}, {}}}}})));

    // registerChildDevices call
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const DeviceRegistrationMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*platformOutboundMessageHandlerMock, addMessage).Times(1);

    // sendOutChildrenSynchronizationRequest call
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const ChildrenSynchronizationRequestMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*registrationProtocolMock,
                getResponseChannelForMessage(MessageType::CHILDREN_SYNCHRONIZATION_REQUEST, _))
      .Times(1);
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, addMessage).Times(1);

    // Callback
    EXPECT_CALL(*gatewayRegistrationProtocolMock, makeOutboundMessage(_, A<const DeviceRegistrationResponseMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*localOutboundMessageHandlerMock, addMessage).Times(1);

    // Call the service and handle the callback
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
    ASSERT_FALSE(service->m_childSyncRequests.empty());
    ASSERT_TRUE(service->m_childSyncRequests.front()->getLambda());
    ASSERT_NO_FATAL_FAILURE(service->m_childSyncRequests.front()->getLambda()(
      std::make_shared<ChildrenSynchronizationResponseMessage>(std::vector<std::string>{"D1"})));
}

TEST_F(DevicesServiceTests, MessageReceivedDeviceRemovalFailsToParse)
{
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType).WillOnce(Return(MessageType::DEVICE_REMOVAL));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(DEVICE_KEY));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, parseDeviceRemovalMessage).WillOnce(Return(ByMove(nullptr)));
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(DevicesServiceTests, MessageReceivedDeviceRemovalFailsToParseTheOutgoingRequest)
{
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType).WillOnce(Return(MessageType::DEVICE_REMOVAL));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(DEVICE_KEY));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, parseDeviceRemovalMessage)
      .WillOnce(Return(ByMove(std::unique_ptr<DeviceRemovalMessage>{new DeviceRemovalMessage{{"Device Key 1"}}})));
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const DeviceRemovalMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(DevicesServiceTests, MessageReceivedDeviceRemovalHappyFlow)
{
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType).WillOnce(Return(MessageType::DEVICE_REMOVAL));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(DEVICE_KEY));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, parseDeviceRemovalMessage)
      .WillOnce(Return(ByMove(std::unique_ptr<DeviceRemovalMessage>{new DeviceRemovalMessage{{"Device Key 1"}}})));
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const DeviceRemovalMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*platformOutboundMessageHandlerMock, addMessage).Times(1);
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(DevicesServiceTests, MessageReceivedRegisteredDevicesRequestFailsToParse)
{
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType)
      .WillOnce(Return(MessageType::REGISTERED_DEVICES_REQUEST));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(DEVICE_KEY));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, parseRegisteredDevicesRequestMessage)
      .WillOnce(Return(ByMove(nullptr)));
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(DevicesServiceTests, MessageReceivedRegisteredDevicesCallbackCalledWithNullptr)
{
    // Handle calls
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType)
      .WillOnce(Return(MessageType::REGISTERED_DEVICES_REQUEST));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(DEVICE_KEY));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, parseRegisteredDevicesRequestMessage)
      .WillOnce(Return(ByMove(std::unique_ptr<RegisteredDevicesRequestMessage>{
        new RegisteredDevicesRequestMessage{std::chrono::milliseconds{1234567890}, {}, {}}})));

    // Send out calls
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const RegisteredDevicesRequestMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*registrationProtocolMock, getResponseChannelForMessage(MessageType::REGISTERED_DEVICES_REQUEST, _))
      .Times(1);
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, addMessage)
      .WillOnce([&](const RetryMessageStruct& retryMessageStruct) { retryMessageStruct.onFail(nullptr); });

    // Call the service
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(DevicesServiceTests, MessageReceivedRegisteredDevicesCallbackCalledWithMessageButFailsToParseForLocalBroker)
{
    // Handle calls
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType)
      .WillOnce(Return(MessageType::REGISTERED_DEVICES_REQUEST));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(DEVICE_KEY));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, parseRegisteredDevicesRequestMessage)
      .WillOnce(Return(ByMove(std::unique_ptr<RegisteredDevicesRequestMessage>{
        new RegisteredDevicesRequestMessage{std::chrono::milliseconds{1234567890}, {}, {}}})));

    // Send out calls
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const RegisteredDevicesRequestMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*registrationProtocolMock, getResponseChannelForMessage(MessageType::REGISTERED_DEVICES_REQUEST, _))
      .Times(1);
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, addMessage).Times(1);
    EXPECT_CALL(*gatewayRegistrationProtocolMock, makeOutboundMessage(_, A<const RegisteredDevicesResponseMessage&>()))
      .WillOnce(Return(ByMove(nullptr)));

    // Call the service
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
    // Find the callback and invoke it
    ASSERT_FALSE(service->m_registeredDevicesRequests.empty());
    const auto params = RegisteredDevicesRequestParameters{std::chrono::milliseconds{1234567890}, {}, {}};
    const auto requestsIt = service->m_registeredDevicesRequests.find(params);
    ASSERT_NE(requestsIt, service->m_registeredDevicesRequests.cend());
    ASSERT_NO_FATAL_FAILURE(requestsIt->second->getLambda()(
      std::make_shared<RegisteredDevicesResponseMessage>(std::chrono::milliseconds{1234567890}, std::string{},
                                                         std::string{}, std::vector<RegisteredDeviceInformation>{})));
}

TEST_F(DevicesServiceTests, MessageReceivedRegisteredDevicesCallbackCalledWithMessageSendsToLocalBroker)
{
    // Handle calls
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getMessageType)
      .WillOnce(Return(MessageType::REGISTERED_DEVICES_REQUEST));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, getDeviceKey).WillOnce(Return(DEVICE_KEY));
    EXPECT_CALL(*gatewayRegistrationProtocolMock, parseRegisteredDevicesRequestMessage)
      .WillOnce(Return(ByMove(std::unique_ptr<RegisteredDevicesRequestMessage>{
        new RegisteredDevicesRequestMessage{std::chrono::milliseconds{1234567890}, {}, {}}})));

    // Send out calls
    EXPECT_CALL(*registrationProtocolMock, makeOutboundMessage(_, A<const RegisteredDevicesRequestMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*registrationProtocolMock, getResponseChannelForMessage(MessageType::REGISTERED_DEVICES_REQUEST, _))
      .Times(1);
    EXPECT_CALL(*platformOutboundRetryMessageHandlerMock, addMessage).Times(1);
    EXPECT_CALL(*gatewayRegistrationProtocolMock, makeOutboundMessage(_, A<const RegisteredDevicesResponseMessage&>()))
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(*localOutboundMessageHandlerMock, addMessage).Times(1);

    // Call the service
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
    // Find the callback and invoke it
    ASSERT_FALSE(service->m_registeredDevicesRequests.empty());
    const auto params = RegisteredDevicesRequestParameters{std::chrono::milliseconds{1234567890}, {}, {}};
    const auto requestsIt = service->m_registeredDevicesRequests.find(params);
    ASSERT_NE(requestsIt, service->m_registeredDevicesRequests.cend());
    ASSERT_NO_FATAL_FAILURE(requestsIt->second->getLambda()(
      std::make_shared<RegisteredDevicesResponseMessage>(std::chrono::milliseconds{1234567890}, std::string{},
                                                         std::string{}, std::vector<RegisteredDeviceInformation>{})));
}
