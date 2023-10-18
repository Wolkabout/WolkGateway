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
#include "gateway/connectivity/GatewayMessageRouter.h"
#undef private
#undef protected

#include "core/utility/Logger.h"
#include "tests/mocks/GatewayMessageListenerMock.h"
#include "tests/mocks/GatewaySubdeviceProtocolMock.h"

#include <gtest/gtest.h>

using namespace wolkabout;
using namespace wolkabout::gateway;
using namespace ::testing;

class GatewayMessageRouterTests : public Test
{
public:
    static void SetUpTestCase() { Logger::init(LogLevel::TRACE, Logger::Type::CONSOLE); }

    void SetUp() override
    {
        service = std::unique_ptr<GatewayMessageRouter>{new GatewayMessageRouter{m_gatewaySubdeviceProtocolMock}};
    }

    std::unique_ptr<GatewayMessageRouter> service;

    GatewaySubdeviceProtocolMock m_gatewaySubdeviceProtocolMock;
};

TEST_F(GatewayMessageRouterTests, CheckProtocol)
{
    EXPECT_EQ(&m_gatewaySubdeviceProtocolMock, &(service->getProtocol()));
}

TEST_F(GatewayMessageRouterTests, AddListenerWithNoMessageTypes)
{
    auto listener = std::make_shared<NiceMock<GatewayMessageListenerMock>>();
    EXPECT_CALL(*listener, getMessageTypes).WillOnce(Return(std::vector<MessageType>{}));
    ASSERT_NO_FATAL_FAILURE(service->addListener("TestListener", std::move(listener)));
    EXPECT_TRUE(service->m_listeners.empty());
}

TEST_F(GatewayMessageRouterTests, AddListenerWithSomeTypes)
{
    // Define the listener and the types
    auto types = std::vector<MessageType>{MessageType::FEED_VALUES, MessageType::PARAMETER_SYNC};
    auto listener = std::make_shared<NiceMock<GatewayMessageListenerMock>>();
    EXPECT_CALL(*listener, getMessageTypes).WillOnce(Return(types));
    ASSERT_NO_FATAL_FAILURE(service->addListener("TestListener", listener));
    // Check the listener, and check that it is listed for those types
    EXPECT_FALSE(service->m_listeners.empty());
    EXPECT_EQ(service->m_listenersPerType[types[0]].lock(), listener);
    EXPECT_EQ(service->m_listenersPerType[types[1]].lock(), listener);
}

TEST_F(GatewayMessageRouterTests, ReceivedMessageInvalidType)
{
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).WillOnce(Return(MessageType::UNKNOWN));
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(GatewayMessageRouterTests, ReceivedMessageNoListener)
{
    ASSERT_TRUE(service->m_listenersPerType.empty());
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).WillOnce(Return(MessageType::FEED_VALUES));
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(GatewayMessageRouterTests, ReceivedMessageExpiredListener)
{
    // Add listener that will expire
    auto types = std::vector<MessageType>{MessageType::FEED_VALUES};
    auto listener = std::make_shared<NiceMock<GatewayMessageListenerMock>>();
    EXPECT_CALL(*listener, getMessageTypes).WillOnce(Return(types));
    ASSERT_NO_FATAL_FAILURE(service->addListener("TestListener", listener));
    EXPECT_FALSE(service->m_listenersPerType.empty());
    // Set up the message receive
    listener.reset();
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).WillOnce(Return(MessageType::FEED_VALUES));
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
    EXPECT_TRUE(service->m_listenersPerType.empty());
}

TEST_F(GatewayMessageRouterTests, ReceivedMessageFoundListenerButFailedToParse)
{
    // Add listener
    auto types = std::vector<MessageType>{MessageType::FEED_VALUES};
    auto listener = std::make_shared<NiceMock<GatewayMessageListenerMock>>();
    EXPECT_CALL(*listener, getMessageTypes).WillOnce(Return(types));
    ASSERT_NO_FATAL_FAILURE(service->addListener("TestListener", listener));
    // Set up the message receive
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).WillOnce(Return(MessageType::FEED_VALUES));
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, parseIncomingSubdeviceMessage)
      .WillOnce(Return(std::vector<GatewaySubdeviceMessage>{}));
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));
}

TEST_F(GatewayMessageRouterTests, ReceivedMessageHappyFlow)
{
    // Add listener
    auto types = std::vector<MessageType>{MessageType::FEED_VALUES};
    auto listener = std::make_shared<NiceMock<GatewayMessageListenerMock>>();
    EXPECT_CALL(*listener, getMessageTypes).WillOnce(Return(types));
    ASSERT_NO_FATAL_FAILURE(service->addListener("TestListener", listener));

    // Now we need to set up the callback in the listener
    std::atomic_bool called{false};
    std::mutex mutex;
    std::condition_variable conditionVariable;
    EXPECT_CALL(*listener, receiveMessages).WillOnce([&](const std::vector<GatewaySubdeviceMessage>& messages) {
        if (!messages.empty())
        {
            called = true;
            conditionVariable.notify_one();
        }
    });

    // Set up the message receive
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, getMessageType).WillOnce(Return(MessageType::FEED_VALUES));
    EXPECT_CALL(m_gatewaySubdeviceProtocolMock, parseIncomingSubdeviceMessage)
      .WillOnce(Return(std::vector<GatewaySubdeviceMessage>{GatewaySubdeviceMessage{wolkabout::Message{"", ""}}}));
    ASSERT_NO_FATAL_FAILURE(service->messageReceived(std::make_shared<wolkabout::Message>("", "")));

    // Now check the callback
    if (!called)
    {
        std::unique_lock<std::mutex> lock{mutex};
        conditionVariable.wait_for(lock, std::chrono::milliseconds{100});
    }
    EXPECT_TRUE(called);
}
