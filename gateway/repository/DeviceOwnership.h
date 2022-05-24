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

#ifndef WOLKGATEWAY_DEVICEOWNERSHIP_H
#define WOLKGATEWAY_DEVICEOWNERSHIP_H

#include <string>

namespace wolkabout
{
namespace gateway
{
// This enum value describes who the device belongs to.
enum class DeviceOwnership
{
    None = -1,
    Platform,
    Gateway
};

/**
 * This is a utility method that is used to convert the enumeration value into a string.
 *
 * @param deviceOwnership The DeviceOwnership value.
 * @return The string representation of the value.
 */
std::string toString(DeviceOwnership deviceOwnership);

/**
 * This is a utility method that is used to convert a string into the enumeration value.
 *
 * @param value A string value.
 * @return A DeviceOwnership value. If the value could not be parsed, will always be `DeviceOwnership::None`.
 */
DeviceOwnership deviceOwnershipFromString(const std::string& value);
}    // namespace gateway
}    // namespace wolkabout

#endif    // WOLKGATEWAY_DEVICEOWNERSHIP_H
