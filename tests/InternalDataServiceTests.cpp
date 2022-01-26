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
#include "gateway/service/internal_data/InternalDataService.h"
#undef private
#undef protected

#include "core/utilities/Logger.h"
#include "tests/mocks/OutboundMessageHandlerMock.h"
#include "tests/mocks/GatewaySubdeviceProtocolMock.h"

#include <gtest/gtest.h>

using namespace wolkabout;
using namespace wolkabout::gateway;
using namespace ::testing;

class InternalDataServiceTests : public Test
{
public:
    static void SetUpTestCase() { Logger::init(LogLevel::TRACE, Logger::Type::CONSOLE); }

    void SetUp() override
    {
        service = std::unique_ptr<InternalDataService>{
          new InternalDataService{GATEWAY_KEY, m_platformOutboundMessageHandlerMock, m_localOutboundMessageHandlerMock,
                                  m_gatewaySubdeviceProtocolMock}};
    }

    static const std::vector<MessageType> messageTypes;

    std::unique_ptr<InternalDataService> service;

    const std::string GATEWAY_KEY = "TEST_GATEWAY";

    OutboundMessageHandlerMock m_platformOutboundMessageHandlerMock;

    OutboundMessageHandlerMock m_localOutboundMessageHandlerMock;

    GatewaySubdeviceProtocolMock m_gatewaySubdeviceProtocolMock;
};

const std::vector<MessageType> InternalDataServiceTests::messageTypes = {MessageType::FEED_VALUES,
                                                                         MessageType::PARAMETER_SYNC,
                                                                         MessageType::TIME_SYNC,
                                                                         MessageType::FILE_UPLOAD_INIT,
                                                                         MessageType::FILE_UPLOAD_ABORT,
                                                                         MessageType::FILE_BINARY_RESPONSE,
                                                                         MessageType::FILE_URL_DOWNLOAD_INIT,
                                                                         MessageType::FILE_URL_DOWNLOAD_ABORT,
                                                                         MessageType::FILE_LIST_REQUEST,
                                                                         MessageType::FILE_DELETE,
                                                                         MessageType::FILE_PURGE,
                                                                         MessageType::FIRMWARE_UPDATE_INSTALL,
                                                                         MessageType::FIRMWARE_UPDATE_ABORT};

TEST_F(InternalDataServiceTests, CheckProtocol)
{
    EXPECT_EQ(&m_gatewaySubdeviceProtocolMock, &(service->getProtocol()));
}

TEST_F(InternalDataServiceTests, GetMessageTypes)
{
    // Obtain the list of message types
    auto types = std::vector<MessageType>{};
    ASSERT_NO_FATAL_FAILURE(types = service->getMessageTypes());

    // Check all the types
    for (const auto& type : messageTypes)
    {
        auto it = std::find(types.cbegin(), types.cend(), type);
        ASSERT_TRUE(it != types.cend());
    }
}

TEST_F(InternalDataServiceTests, ReceivedMessageFailedToParseMessage)
{
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, makeOutboundMessage).WillOnce(Return(ByMove(nullptr)));
    EXPECT_CALL(m_platformOutboundMessageHandlerMock, addMessage).Times(0);
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(InternalDataServiceTests, ReceivedMessageHappyFlow)
{
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, makeOutboundMessage)
      .WillOnce(Return(ByMove(std::unique_ptr<wolkabout::Message>{new wolkabout::Message{"", ""}})));
    EXPECT_CALL(m_platformOutboundMessageHandlerMock, addMessage).Times(1);
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(InternalDataServiceTests, ReceiveMessagesOneMessage)
{
    // Define the message
    auto message = std::vector<GatewaySubdeviceMessage>{GatewaySubdeviceMessage{wolkabout::Message{"", ""}}};
    EXPECT_CALL(m_localOutboundMessageHandlerMock, addMessage).Times(1);
    ASSERT_NO_FATAL_FAILURE(service->receiveMessages(message));
}

TEST_F(InternalDataServiceTests, ReceiveMessagesFiveMessage)
{
    // Define the message
    auto messages = std::vector<GatewaySubdeviceMessage>{};
    for (auto i = 0; i < 5; ++i)
        messages.emplace_back(GatewaySubdeviceMessage{wolkabout::Message{"", ""}});
    EXPECT_CALL(m_localOutboundMessageHandlerMock, addMessage).Times(5);
    ASSERT_NO_FATAL_FAILURE(service->receiveMessages(messages));
}
