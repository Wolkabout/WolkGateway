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

#include "connectivity/json/JsonProtocol.h"
#include <string>

namespace wolkabout
{
const std::string JSON_PROTOCOL = "JsonProtocol";

#define FIRST_ARG(...) FIRST_ARG_(__VA_ARGS__, ignored)
#define FIRST_ARG_(first, ...) first

#define REST_ARGS_1(ignored)
#define REST_ARGS_2_OR_MORE(ignored, ...) __VA_ARGS__

#define GET_4TH_ARG(arg1, arg2, arg3, arg4, ...) arg4
#define GET_REST_ARGS_OVERRIDE(...) \
    GET_4TH_ARG(__VA_ARGS__, REST_ARGS_2_OR_MORE, REST_ARGS_2_OR_MORE, REST_ARGS_1, ignored)

#define REST_ARGS(...) GET_REST_ARGS_OVERRIDE(__VA_ARGS__)(__VA_ARGS__)

#define MapProtocol(...)                                                         \
    [&](const std::string& name) {                                               \
        if (name == JSON_PROTOCOL)                                               \
            return FIRST_ARG(__VA_ARGS__)<JsonProtocol>(REST_ARGS(__VA_ARGS__)); \
        else                                                                     \
            return FIRST_ARG(__VA_ARGS__)<void>(REST_ARGS(__VA_ARGS__));         \
    }
}    // namespace wolkabout

#endif
