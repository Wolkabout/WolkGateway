/**
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

#ifndef FEEDUPDATEHANDLER_H
#define FEEDUPDATEHANDLER_H

#include "core/model/Reading.h"

#include <map>
#include <string>

namespace wolkabout
{
/**
 * This interface describes an object that can receive feed value updates.
 */
class FeedUpdateHandler
{
public:
    /**
     * Default virtual destructor.
     */
    virtual ~FeedUpdateHandler() = default;

    /**
     * This method will be invoked once new values have been received.
     *
     * @param readings All feed readings that have been updated. Grouped up by time when a reading change happened. Key
     * is an epoch timestamp in milliseconds, and value is an array of readings changed at that time.
     */
    virtual void handleUpdate(std::map<std::uint64_t, std::vector<Reading>> readings) = 0;
};
}    // namespace wolkabout

#endif
