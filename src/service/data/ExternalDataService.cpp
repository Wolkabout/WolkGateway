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

#include "ExternalDataService.h"

#include "core/model/Message.h"
#include "core/protocol/DataProtocol.h"
#include "core/utilities/Logger.h"

#include <algorithm>

namespace wolkabout
{
void ExternalDataService::addReading(const std::string& deviceKey, const Reading& reading)
{
    addReadings(deviceKey, {reading});
}

void ExternalDataService::addReadings(const std::string& deviceKey, const std::vector<Reading>& readings)
{
    if (readings.empty())
    {
        return;
    }

    const std::shared_ptr<Message> message = m_protocol.makeOutboundMessage(deviceKey, FeedValuesMessage{readings});

    addMessage(message);
}

void ExternalDataService::handleMessageForDevice(std::shared_ptr<Message> /*message*/)
{
    LOG(WARN) << "Not handling message for device";
}

}    // namespace wolkabout
