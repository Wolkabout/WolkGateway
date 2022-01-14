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

#ifndef GATEWAYDATASERVICE_H
#define GATEWAYDATASERVICE_H

#include "core/InboundMessageHandler.h"
#include "core/model/Reading.h"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class DataProtocol;
class Persistence;
class ConnectivityService;
class OutboundMessageHandler;

typedef std::function<void(std::map<std::uint64_t, std::vector<Reading>>)> FeedUpdateHandler;

class GatewayDataService : public MessageListener
{
public:
    GatewayDataService(std::string deviceKey, DataProtocol& protocol, Persistence& persistence,
                       OutboundMessageHandler& outboundMessageHandler, const FeedUpdateHandler& feedUpdateHandler);

    void messageReceived(std::shared_ptr<Message> message) override;
    const Protocol& getProtocol() override;

    void addReading(const std::string& reference, const std::string& value, std::uint64_t rtc);

    void addReading(const std::string& reference, const std::vector<std::string>& values, std::uint64_t rtc);

    void publishReadings();

private:
    void publishReadingsForPersistanceKey(const std::string& persistanceKey);

    const std::string m_deviceKey;

    DataProtocol& m_protocol;
    Persistence& m_persistence;
    OutboundMessageHandler& m_outboundMessageHandler;

    FeedUpdateHandler m_feedUpdateHandler;

    static const constexpr unsigned int PUBLISH_BATCH_ITEMS_COUNT = 50;
};
}    // namespace wolkabout

#endif
