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

#ifndef WOLKGATEWAY_DEVICEFILTER_H
#define WOLKGATEWAY_DEVICEFILTER_H

#include <string>

namespace wolkabout
{
namespace gateway
{
/**
 * This interface defines an object that is capable of filtering through devices that actually exist and can have data
 * sent about them, and the ones that can not.
 */
class DeviceFilter
{
public:
    /**
     * Default virtual destructor.
     */
    virtual ~DeviceFilter() = default;

    /**
     * This is the filtration method that determines whether information about device exists, or not.
     *
     * @param deviceKey The key of the device.
     * @return Whether the device exists or not.
     */
    virtual bool deviceExists(const std::string& deviceKey) = 0;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // WOLKGATEWAY_DEVICEFILTER_H
