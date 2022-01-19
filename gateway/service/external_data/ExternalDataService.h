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

#include "core/MessageListener.h"
#include "core/connectivity/OutboundMessageHandler.h"
#include "core/protocol/DataProtocol.h"
#include "core/protocol/GatewaySubdeviceProtocol.h"
#include "gateway/api/DataHandler.h"

namespace wolkabout
{
namespace gateway
{
class ExternalDataService : public DataHandler, public MessageListener
{
public:
    void addReading(const std::string& deviceKey, const Reading& reading) override;

    void addReadings(const std::string& deviceKey, const std::vector<Reading>& readings) override;

    void pullFeedValues(const std::string& deviceKey) override;

    void pullParameters(const std::string& deviceKey) override;

    void registerFeed(const std::string& deviceKey, const Feed& feed) override;

    void registerFeeds(const std::string& deviceKey, const std::vector<Feed>& feeds) override;

    void removeFeed(const std::string& deviceKey, const std::string& reference) override;

    void removeFeeds(const std::string& deviceKey, const std::vector<std::string>& references) override;

    void addAttribute(const std::string& deviceKey, Attribute attribute) override;

    void updateParameter(const std::string& deviceKey, Parameter parameters) override;

    void messageReceived(std::shared_ptr<Message> message) override;

    const Protocol& getProtocol() override;

private:
    // Here we will store the protocols necessary to generate the messages
    GatewaySubdeviceProtocol& m_gatewaySubdeviceProtocol;
    DataProtocol& m_dataProtocol;

    // Here we will store the handler used to send out the messages
    OutboundMessageHandler& m_outboundMessageHandler;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // WOLKABOUT_EXTERNALDATASERVICE_H
