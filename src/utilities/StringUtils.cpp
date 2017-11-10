/*
 * Copyright 2017 WolkAbout Technology s.r.o.
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

#include "utilities/StringUtils.h"

#include <string>
#include <vector>

namespace wolkabout
{
bool StringUtils::contains(const std::string& string, char c)
{
    return string.find(c) != std::string::npos;
}

std::vector<std::string> StringUtils::tokenize(const std::string& string, const std::string& delimiters)
{
    std::vector<std::string> tokens;
    if (string.empty())
    {
        return tokens;
    }

    std::string::size_type position = 0;
    std::string::size_type delimiterPosition = string.find_first_of(delimiters, position);

    while (std::string::npos != delimiterPosition)
    {
        tokens.push_back(string.substr(position, delimiterPosition - position));
        position = delimiterPosition + 1;
        delimiterPosition = string.find_first_of(delimiters, position);
    }

    tokens.push_back(string.substr(position, string.size() - position));
    return tokens;
}

bool StringUtils::endsWith(const std::string& string, const std::string& suffix)
{
    return string.size() >= suffix.size() && 0 == string.compare(string.size() - suffix.size(), suffix.size(), suffix);
}
}
