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
#ifndef WOLKGATEWAY_DATAPROVIDERMOCK_H
#define WOLKGATEWAY_DATAPROVIDERMOCK_H

#include "gateway/api/DataProvider.h"

#include <gmock/gmock.h>

using namespace wolkabout;
using namespace wolkabout::gateway;

class DataProviderMock : public DataProvider
{
public:
    MOCK_METHOD(void, setDataHandler, (DataHandler*, const std::string&));
    MOCK_METHOD(void, receiveReadingData, (const std::string&, (std::map<std::uint64_t, std::vector<Reading>>)));
    MOCK_METHOD(void, receiveParameterData, (const std::string&, std::vector<Parameter>));
};

#endif    // WOLKGATEWAY_DATAPROVIDERMOCK_H
