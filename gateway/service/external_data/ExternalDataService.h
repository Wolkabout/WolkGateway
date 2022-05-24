/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#ifndef WOLKABOUT_EXTERNALDATASERVICE_H
#define WOLKABOUT_EXTERNALDATASERVICE_H

#include "core/utilities/CommandBuffer.h"
#include "gateway/GatewayMessageListener.h"
#include "gateway/api/DataHandler.h"
#include "gateway/api/DataProvider.h"

namespace wolkabout
{
class DataProtocol;
class GatewaySubdeviceProtocol;
class OutboundMessageHandler;

namespace gateway
{
class ExternalDataService : public DataHandler, public GatewayMessageListener
{
public:
    ExternalDataService(std::string gatewayKey, GatewaySubdeviceProtocol& gatewaySubdeviceProtocol,
                        DataProtocol& dataProtocol, OutboundMessageHandler& outboundMessageHandler,
                        DataProvider& dataProvider);

    std::vector<MessageType> getMessageTypes() const override;

    void receiveMessages(const std::vector<GatewaySubdeviceMessage>& messages) override;

    void addReading(const std::string& deviceKey, const Reading& reading) override;

    void addReadings(const std::string& deviceKey, const std::vector<Reading>& readings) override;

    void pullFeedValues(const std::string& deviceKey) override;

    void pullParameters(const std::string& deviceKey) override;

    void registerFeed(const std::string& deviceKey, const Feed& feed) override;

    void registerFeeds(const std::string& deviceKey, const std::vector<Feed>& feeds) override;

    void removeFeed(const std::string& deviceKey, const std::string& reference) override;

    void removeFeeds(const std::string& deviceKey, const std::vector<std::string>& references) override;

    void addAttribute(const std::string& deviceKey, Attribute attribute) override;

    void updateParameter(const std::string& deviceKey, Parameter parameter) override;

private:
    void packMessageWithGatewayAndSend(const Message& message);

    // Logger tag
    const std::string TAG = "[ExternalDataService] -> ";

    // We need the gateway key
    std::string m_gatewayKey;

    // Here we will store the protocols necessary to generate the messages
    GatewaySubdeviceProtocol& m_gatewaySubdeviceProtocol;
    DataProtocol& m_dataProtocol;

    // Here we will store the handler used to send out the messages
    OutboundMessageHandler& m_outboundMessageHandler;

    // And this is the external data provider.
    DataProvider& m_dataProvider;

    // The command buffer for asynchronous execution
    CommandBuffer m_commandBuffer;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // WOLKABOUT_EXTERNALDATASERVICE_H
