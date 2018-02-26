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

#ifndef PROTOCOLMAPPER_H
#define PROTOCOLMAPPER_H

#include "connectivity/json/JsonSingleProtocol.h"
#include <string>

namespace wolkabout
{
const std::string JSON_SINGLE_PROTOCOL = "JsonSingle";

#define MapProtocol(name, func)                \
    [&] {                                      \
        if (name == JSON_SINGLE_PROTOCOL)      \
            return func<JsonSingleProtocol>(); \
        else                                   \
            return func<void>();               \
    }()
}

#endif
