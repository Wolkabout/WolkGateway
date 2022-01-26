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

#ifndef WOLKGATEWAY_DATAHANDLERMOCK_H
#define WOLKGATEWAY_DATAHANDLERMOCK_H

#include "gateway/api/DataHandler.h"

#include <gmock/gmock.h>

using namespace wolkabout;
using namespace wolkabout::gateway;

class DataHandlerMock : public DataHandler
{
public:
    MOCK_METHOD(void, addReading, (const std::string&, const Reading&));
    MOCK_METHOD(void, addReadings, (const std::string&, const std::vector<Reading>& s));
    MOCK_METHOD(void, pullFeedValues, (const std::string&));
    MOCK_METHOD(void, pullParameters, (const std::string&));
    MOCK_METHOD(void, registerFeed, (const std::string&, const Feed&));
    MOCK_METHOD(void, registerFeeds, (const std::string&, const std::vector<Feed>&));
    MOCK_METHOD(void, removeFeed, (const std::string&, const std::string&));
    MOCK_METHOD(void, removeFeeds, (const std::string&, const std::vector<std::string>&));
    MOCK_METHOD(void, addAttribute, (const std::string&, Attribute));
    MOCK_METHOD(void, updateParameter, (const std::string&, Parameter));
};

#endif    // WOLKGATEWAY_DATAHANDLERMOCK_H
