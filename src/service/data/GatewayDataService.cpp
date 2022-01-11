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

#include "GatewayDataService.h"

#include "OutboundMessageHandler.h"
#include "core/connectivity/ConnectivityService.h"
#include "core/model/Message.h"
#include "core/model/Reading.h"
#include "core/persistence/Persistence.h"
#include "core/protocol/DataProtocol.h"
#include "core/utilities/Logger.h"

#include <algorithm>
#include <cassert>

namespace wolkabout
{
GatewayDataService::GatewayDataService(std::string deviceKey, DataProtocol& protocol, Persistence& persistence,
                                       OutboundMessageHandler& outboundMessageHandler,
                                       const FeedUpdateHandler& feedUpdateHandler)
: m_deviceKey{std::move(deviceKey)}
, m_protocol{protocol}
, m_persistence{persistence}
, m_outboundMessageHandler{outboundMessageHandler}
, m_feedUpdateHandler{feedUpdateHandler}
{
}

void GatewayDataService::messageReceived(std::shared_ptr<Message> message)
{
    assert(message);

    const std::string deviceKey = m_protocol.getDeviceKey(*message);
    if (deviceKey.empty())
    {
        LOG(WARN) << "Unable to extract device key from channel: " << message->getChannel();
        return;
    }

    if (deviceKey != m_deviceKey)
    {
        LOG(WARN) << "Device key mismatch: " << message->getChannel();
        return;
    }

    switch (m_protocol.getMessageType(*message))
    {
    case MessageType::FEED_VALUES:
    {
        auto feedValuesMessage = m_protocol.parseFeedValues(message);
        if (feedValuesMessage == nullptr)
        {
            LOG(WARN) << "Unable to parse message: " << message->getChannel();
        }
        else if (m_feedUpdateHandler)
        {
            m_feedUpdateHandler(feedValuesMessage->getReadings());
        }

        break;
    }
    case MessageType::PARAMETER_SYNC:
    {
        auto parameterMessage = m_protocol.parseParameters(message);
        if (parameterMessage == nullptr)
        {
            LOG(WARN) << "Unable to parse message: " << message->getChannel();
            return;
        }

        // TODO
        break;
    }
    default:
    {
        LOG(WARN) << "Unable to parse message channel: " << message->getChannel();
    }
    }
}

const Protocol& GatewayDataService::getProtocol()
{
    return m_protocol;
}

void GatewayDataService::addReading(const std::string& reference, const std::string& value, std::uint64_t rtc)
{
    auto reading = Reading(reference, value, rtc);

    m_persistence.putReading(reference, reading);
}

void GatewayDataService::addReading(const std::string& reference, const std::vector<std::string>& values,
                                    std::uint64_t rtc)
{
    auto reading = Reading(reference, values, rtc);

    m_persistence.putReading(reference, reading);
}

void GatewayDataService::publishReadings()
{
    for (const auto& key : m_persistence.getReadingsKeys())
    {
        publishReadingsForPersistanceKey(key);
    }
}

void GatewayDataService::publishReadingsForPersistanceKey(const std::string& persistanceKey)
{
    const auto readings = m_persistence.getReadings(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);

    if (readings.empty())
    {
        return;
    }

    // TODO shared?
    const std::shared_ptr<Message> outboundMessage =
      nullptr;    // m_protocol.makeOutboundMessage(m_deviceKey, FeedValuesMessage{readings});

    m_persistence.removeReadings(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);

    if (!outboundMessage)
    {
        LOG(ERROR) << "Unable to create message from readings: " << persistanceKey;
        return;
    }

    m_outboundMessageHandler.addMessage(outboundMessage);

    publishReadingsForPersistanceKey(persistanceKey);
}
}    // namespace wolkabout
