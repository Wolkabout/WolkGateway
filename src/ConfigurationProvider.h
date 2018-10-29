/*
 * Copyright 2018 WolkAbout Technology s.r.o.
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

#ifndef CONFIGURATIONPROVIDER_H
#define CONFIGURATIONPROVIDER_H

#include "model/ConfigurationItem.h"

#include <vector>

namespace wolkabout
{
class ConfigurationProvider
{
public:
    /**
     * @brief Device configuration provider callback
     *        Reads device configuration and returns it as
     *        vector of wolkabout::ConfigurationItem.<br>
     *
     *        Must be implemented as non blocking<br>
     *        Must be implemented as thread safe
     * @return Device configuration as std::vector<ConfigurationItem>
     */
    virtual std::vector<ConfigurationItem> getConfiguration() = 0;

    virtual ~ConfigurationProvider() = default;
};
}    // namespace wolkabout

#endif
