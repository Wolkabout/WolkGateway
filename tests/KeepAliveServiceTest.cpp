/*
 * Copyright 2018 WolkAbout Technology s.r.o.
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

#include "OutboundMessageHandler.h"
#include "protocol/json/JsonStatusProtocol.h"
#include "service/KeepAliveService.h"

#include <gtest/gtest.h>
#include <memory>

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

class KeepAliveService : public ::testing::Test
{
public:
    void SetUp() override
    {
        protocol.reset(new wolkabout::JsonStatusProtocol());
        platformOutboundMessageHandler =
          std::unique_ptr<PlatformOutboundMessageHandler>(new PlatformOutboundMessageHandler());
        keepAliveService = std::unique_ptr<wolkabout::KeepAliveService>(
          new wolkabout::KeepAliveService(GATEWAY_KEY, *protocol, *platformOutboundMessageHandler, PING_INTERVAL));
    }

    void TearDown() override {}

    std::unique_ptr<wolkabout::StatusProtocol> protocol;
    std::unique_ptr<PlatformOutboundMessageHandler> platformOutboundMessageHandler;
    std::unique_ptr<wolkabout::KeepAliveService> keepAliveService;

    static constexpr const std::chrono::seconds PING_INTERVAL{5};
    static constexpr const char* GATEWAY_KEY = "gateway_key";
};
}    // namespace

TEST_F(KeepAliveService, Given_When_ConnectedIsCalled_Then_PingMessageIsSentToPlatform)
{
    // Given
    // Intentionally left empty

    // When
    keepAliveService->connected();

    // Then
    ASSERT_EQ(platformOutboundMessageHandler->getMessages().size(), 1);
}
