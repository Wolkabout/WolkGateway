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

#ifndef WOLKABOUT_DATAHANDLER_H
#define WOLKABOUT_DATAHANDLER_H

#include "core/model/Reading.h"

#include <string>
#include <vector>

namespace wolkabout
{
class DataHandler
{
public:
    virtual ~DataHandler() = default;

    virtual void addReading(const std::string& deviceKey, const Reading& reading) = 0;
    virtual void addReadings(const std::string& deviceKey, const std::vector<Reading>& readings) = 0;
};
}    // namespace wolkabout

#endif    // WOLKABOUT_DATAHANDLER_H
