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
#include "gateway/service/external_data/ExternalDataService.h"
#undef private
#undef protected

#include "core/utilities/Logger.h"
#include "tests/mocks/DataProtocolMock.h"
#include "tests/mocks/DataProviderMock.h"
#include "tests/mocks/GatewaySubdeviceProtocolMock.h"
#include "tests/mocks/OutboundMessageHandlerMock.h"

#include <gtest/gtest.h>

using namespace wolkabout;
using namespace wolkabout::gateway;
using namespace ::testing;

class ExternalDataServiceTests : public Test
{
public:
    static void SetUpTestCase() { Logger::init(LogLevel::TRACE, Logger::Type::CONSOLE); }

    void SetUp() override
    {
        service = std::unique_ptr<ExternalDataService>{
          new ExternalDataService{GATEWAY_KEY, m_gatewaySubdeviceProtocolMock, m_dataProtocolMock,
                                  m_platformOutboundMessageHandler, m_dataProviderMock}};
    }

    template <class T> void MakeOutboundReturnsNull()
    {
        EXPECT_CALL(m_dataProtocolMock, makeOutboundMessage(A<const std::string&>(), A<T>()))
          .WillOnce(Return(ByMove(nullptr)));
    }

    template <class T> void MakeOutboundReturnsMessage()
    {
        EXPECT_CALL(m_dataProtocolMock, makeOutboundMessage(A<const std::string&>(), A<T>()))
          .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    }

    void PublishNotCalled() { EXPECT_CALL(m_platformOutboundMessageHandler, addMessage).Times(0); }

    void SetUpForPackSend()
    {
        EXPECT_CALL(m_gatewaySubdeviceProtocolMock, makeOutboundMessage)
          .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
        EXPECT_CALL(m_platformOutboundMessageHandler, addMessage).Times(1);
    }

    static Reading GenerateReading() { return Reading{"TEST", std::string{"TestValue"}}; }

    static Feed GenerateFeed() { return Feed{"TestFeed", "TF", FeedType::IN_OUT, "NUMERIC"}; }

    static Attribute GenerateAttribute() { return Attribute{"TestAttribute", DataType::STRING, "TestValue"}; }

    static Parameter GenerateParameter() { return Parameter{ParameterName::EXTERNAL_ID, "TestExternalId"}; }

    static std::vector<GatewaySubdeviceMessage> GenerateMessages(std::uint64_t messageCount)
    {
        return std::vector<GatewaySubdeviceMessage>(messageCount, GatewaySubdeviceMessage{wolkabout::Message{"", ""}});
    }

    std::unique_ptr<ExternalDataService> service;

    const std::string GATEWAY_KEY = "TEST_GATEWAY";

    GatewaySubdeviceProtocolMock m_gatewaySubdeviceProtocolMock;

    DataProtocolMock m_dataProtocolMock;

    OutboundMessageHandlerMock m_platformOutboundMessageHandler;

    DataProviderMock m_dataProviderMock;

    const std::vector<MessageType> MESSAGE_TYPE_LIST = {MessageType::FEED_VALUES, MessageType::PARAMETER_SYNC};
};

TEST_F(ExternalDataServiceTests, GetMessageTypes)
{
    // Obtain the list of message types
    auto types = std::vector<MessageType>{};
    ASSERT_NO_FATAL_FAILURE(types = service->getMessageTypes());

    // Check all the types
    for (const auto& type : MESSAGE_TYPE_LIST)
    {
        auto it = std::find(types.cbegin(), types.cend(), type);
        ASSERT_TRUE(it != types.cend());
    }
}

TEST_F(ExternalDataServiceTests, PackMessageParserFails)
{
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, makeOutboundMessage).WillOnce(Return(ByMove(nullptr)));
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->packMessageWithGatewayAndSend({"", ""}));
}

TEST_F(ExternalDataServiceTests, PackMessageHappyFlow)
{
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->packMessageWithGatewayAndSend({"", ""}));
}

TEST_F(ExternalDataServiceTests, AddReadingParserFailes)
{
    MakeOutboundReturnsNull<FeedValuesMessage>();
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->addReading(GATEWAY_KEY, GenerateReading()));
}

TEST_F(ExternalDataServiceTests, AddReadingHappyFlow)
{
    MakeOutboundReturnsMessage<FeedValuesMessage>();
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->addReading(GATEWAY_KEY, GenerateReading()));
}

TEST_F(ExternalDataServiceTests, AddReadingsParserFailes)
{
    MakeOutboundReturnsNull<FeedValuesMessage>();
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->addReadings(GATEWAY_KEY, {GenerateReading()}));
}

TEST_F(ExternalDataServiceTests, AddReadingsHappyFlow)
{
    MakeOutboundReturnsMessage<FeedValuesMessage>();
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->addReadings(GATEWAY_KEY, {GenerateReading()}));
}

TEST_F(ExternalDataServiceTests, PullFeedValuesParserFailes)
{
    MakeOutboundReturnsNull<PullFeedValuesMessage>();
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->pullFeedValues(GATEWAY_KEY));
}

TEST_F(ExternalDataServiceTests, PullFeedValuesHappyFlow)
{
    MakeOutboundReturnsMessage<PullFeedValuesMessage>();
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->pullFeedValues(GATEWAY_KEY));
}

TEST_F(ExternalDataServiceTests, PullParametersParserFailes)
{
    MakeOutboundReturnsNull<ParametersPullMessage>();
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->pullParameters(GATEWAY_KEY));
}

TEST_F(ExternalDataServiceTests, PullParametersHappyFlow)
{
    MakeOutboundReturnsMessage<ParametersPullMessage>();
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->pullParameters(GATEWAY_KEY));
}

TEST_F(ExternalDataServiceTests, RegisterFeedParserFailes)
{
    MakeOutboundReturnsNull<FeedRegistrationMessage>();
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->registerFeed(GATEWAY_KEY, GenerateFeed()));
}

TEST_F(ExternalDataServiceTests, RegisterFeedHappyFlow)
{
    MakeOutboundReturnsMessage<FeedRegistrationMessage>();
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->registerFeed(GATEWAY_KEY, GenerateFeed()));
}

TEST_F(ExternalDataServiceTests, RegisterFeedsParserFailes)
{
    MakeOutboundReturnsNull<FeedRegistrationMessage>();
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->registerFeeds(GATEWAY_KEY, {GenerateFeed()}));
}

TEST_F(ExternalDataServiceTests, RegisterFeedsHappyFlow)
{
    MakeOutboundReturnsMessage<FeedRegistrationMessage>();
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->registerFeeds(GATEWAY_KEY, {GenerateFeed()}));
}

TEST_F(ExternalDataServiceTests, RemoveFeedParserFailes)
{
    MakeOutboundReturnsNull<FeedRemovalMessage>();
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->removeFeed(GATEWAY_KEY, "TestFeed"));
}

TEST_F(ExternalDataServiceTests, RemoveFeedHappyFlow)
{
    MakeOutboundReturnsMessage<FeedRemovalMessage>();
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->removeFeed(GATEWAY_KEY, "TestFeed"));
}

TEST_F(ExternalDataServiceTests, RemoveFeedsParserFailes)
{
    MakeOutboundReturnsNull<FeedRemovalMessage>();
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->removeFeeds(GATEWAY_KEY, {"TestFeed"}));
}

TEST_F(ExternalDataServiceTests, RemoveFeedsHappyFlow)
{
    MakeOutboundReturnsMessage<FeedRemovalMessage>();
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->removeFeeds(GATEWAY_KEY, {"TestFeed"}));
}

TEST_F(ExternalDataServiceTests, AddAttributeParserFailes)
{
    MakeOutboundReturnsNull<AttributeRegistrationMessage>();
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->addAttribute(GATEWAY_KEY, GenerateAttribute()));
}

TEST_F(ExternalDataServiceTests, AddAttributeHappyFlow)
{
    MakeOutboundReturnsMessage<AttributeRegistrationMessage>();
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->addAttribute(GATEWAY_KEY, GenerateAttribute()));
}

TEST_F(ExternalDataServiceTests, UpdateParameterParserFailes)
{
    MakeOutboundReturnsNull<ParametersUpdateMessage>();
    PublishNotCalled();
    ASSERT_NO_FATAL_FAILURE(service->updateParameter(GATEWAY_KEY, GenerateParameter()));
}

TEST_F(ExternalDataServiceTests, UpdateParameterHappyFlow)
{
    MakeOutboundReturnsMessage<ParametersUpdateMessage>();
    SetUpForPackSend();
    ASSERT_NO_FATAL_FAILURE(service->updateParameter(GATEWAY_KEY, GenerateParameter()));
}

TEST_F(ExternalDataServiceTests, ReceiveMessagesEmptyVector)
{
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).Times(0);
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getDeviceKey).Times(0);
    ASSERT_NO_FATAL_FAILURE(service->receiveMessages({}));
}

TEST_F(ExternalDataServiceTests, ReceiveMessagesNotHandledType)
{
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).WillOnce(Return(MessageType::TIME_SYNC));
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getDeviceKey).WillOnce(Return(GATEWAY_KEY));
    EXPECT_CALL(m_dataProtocolMock, parseFeedValues).Times(0);
    EXPECT_CALL(m_dataProtocolMock, parseParameters).Times(0);
    ASSERT_NO_FATAL_FAILURE(service->receiveMessages(GenerateMessages(1)));
}

TEST_F(ExternalDataServiceTests, ReceiveFeedValuesButFailsToParse)
{
    // Set up the service call, and await the callback call
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).WillOnce(Return(MessageType::FEED_VALUES));
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getDeviceKey).WillOnce(Return(GATEWAY_KEY));
    EXPECT_CALL(m_dataProtocolMock, parseFeedValues).WillOnce(Return(nullptr));
    EXPECT_CALL(m_dataProviderMock, onReadingData).Times(0);
    ASSERT_NO_FATAL_FAILURE(service->receiveMessages(GenerateMessages(1)));
}

TEST_F(ExternalDataServiceTests, ReceiveFeedValuesMessage)
{
    // Entities for callback tracking
    std::atomic_bool called{false};
    std::mutex mutex;
    std::condition_variable conditionVariable;
    EXPECT_CALL(m_dataProviderMock, onReadingData)
      .WillOnce([&](const std::string& deviceKey, const std::map<std::uint64_t, std::vector<Reading>>& readings) {
          if (deviceKey == GATEWAY_KEY && !readings.empty())
          {
              called = true;
              conditionVariable.notify_one();
          }
      });
    // Set up the service call, and await the callback call
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).WillOnce(Return(MessageType::FEED_VALUES));
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getDeviceKey).WillOnce(Return(GATEWAY_KEY));
    EXPECT_CALL(m_dataProtocolMock, parseFeedValues)
      .WillOnce(Return(std::make_shared<FeedValuesMessage>(std::vector<Reading>{GenerateReading()})));
    ASSERT_NO_FATAL_FAILURE(service->receiveMessages(GenerateMessages(1)));
    if (!called)
    {
        std::unique_lock<std::mutex> lock{mutex};
        conditionVariable.wait_for(lock, std::chrono::milliseconds{100});
    }
    EXPECT_TRUE(called);
}

TEST_F(ExternalDataServiceTests, ReceiveParametersButFailsToParse)
{
    // Set up the service call, and await the callback call
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).WillOnce(Return(MessageType::PARAMETER_SYNC));
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getDeviceKey).WillOnce(Return(GATEWAY_KEY));
    EXPECT_CALL(m_dataProtocolMock, parseParameters).WillOnce(Return(nullptr));
    EXPECT_CALL(m_dataProviderMock, onParameterData).Times(0);
    ASSERT_NO_FATAL_FAILURE(service->receiveMessages(GenerateMessages(1)));
}

TEST_F(ExternalDataServiceTests, ReceiveParametersMessage)
{
    // Entities for callback tracking
    std::atomic_bool called{false};
    std::mutex mutex;
    std::condition_variable conditionVariable;
    EXPECT_CALL(m_dataProviderMock, onParameterData)
      .WillOnce([&](const std::string& deviceKey, const std::vector<Parameter>& parameters) {
          if (deviceKey == GATEWAY_KEY && !parameters.empty())
          {
              called = true;
              conditionVariable.notify_one();
          }
      });
    // Set up the service call, and await the callback call
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).WillOnce(Return(MessageType::PARAMETER_SYNC));
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getDeviceKey).WillOnce(Return(GATEWAY_KEY));
    EXPECT_CALL(m_dataProtocolMock, parseParameters)
      .WillOnce(Return(std::make_shared<ParametersUpdateMessage>(std::vector<Parameter>{GenerateParameter()})));
    ASSERT_NO_FATAL_FAILURE(service->receiveMessages(GenerateMessages(1)));
    if (!called)
    {
        std::unique_lock<std::mutex> lock{mutex};
        conditionVariable.wait_for(lock, std::chrono::milliseconds{100});
    }
    EXPECT_TRUE(called);
}
