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

#ifndef WOLKABOUT_DATAPROVIDER_H
#define WOLKABOUT_DATAPROVIDER_H

#include "gateway/api/DataHandler.h"

#include <map>

namespace wolkabout::gateway
{
class DataProvider
{
public:
    virtual ~DataProvider() = default;

    virtual void setDataHandler(DataHandler* handler, const std::string& gatewayKey) = 0;

    virtual void onReadingData(const std::string& deviceKey,
                               std::map<std::uint64_t, std::vector<Reading>> readings) = 0;

    virtual void onParameterData(const std::string& deviceKey, std::vector<Parameter> parameters) = 0;
};
}    // namespace wolkabout::gateway

#endif    // WOLKABOUT_DATAPROVIDER_H
