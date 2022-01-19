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
namespace gateway
{
void ExternalDataService::addReading(const std::string& deviceKey, const Reading& reading) {}

void ExternalDataService::addReadings(const std::string& deviceKey, const std::vector<Reading>& readings) {}

void ExternalDataService::pullFeedValues(const std::string& deviceKey) {}

void ExternalDataService::pullParameters(const std::string& deviceKey) {}

void ExternalDataService::registerFeed(const std::string& deviceKey, const Feed& feed) {}

void ExternalDataService::registerFeeds(const std::string& deviceKey, const std::vector<Feed>& feeds) {}

void ExternalDataService::removeFeed(const std::string& deviceKey, const std::string& reference) {}

void ExternalDataService::removeFeeds(const std::string& deviceKey, const std::vector<std::string>& references) {}

void ExternalDataService::addAttribute(const std::string& deviceKey, Attribute attribute) {}

void ExternalDataService::updateParameter(const std::string& deviceKey, Parameter parameters) {}

void ExternalDataService::messageReceived(std::shared_ptr<Message> message) {}

const Protocol& ExternalDataService::getProtocol()
{
    return <#initializer #>;
}
}    // namespace gateway
}    // namespace wolkabout
