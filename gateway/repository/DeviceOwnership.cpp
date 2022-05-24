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

#include "gateway/repository/DeviceOwnership.h"

namespace wolkabout
{
namespace gateway
{
std::string toString(DeviceOwnership deviceOwnership)
{
    switch (deviceOwnership)
    {
    case DeviceOwnership::Platform:
        return "Platform";
    case DeviceOwnership::Gateway:
        return "Gateway";
    default:
        return {};
    }
}

DeviceOwnership deviceOwnershipFromString(const std::string& value)
{
    if (value == "Platform")
        return DeviceOwnership::Platform;
    else if (value == "Gateway")
        return DeviceOwnership::Gateway;
    return DeviceOwnership::None;
}
}    // namespace gateway
}    // namespace wolkabout
