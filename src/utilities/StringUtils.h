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

#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <string>
#include <vector>

namespace wolkabout
{
class StringUtils
{
public:
    StringUtils() = delete;

    static bool contains(const std::string& string, char c);

    static bool contains(const std::string& string, const std::string& substring);

    static std::vector<std::string> tokenize(const std::string& string, const std::string& delimiters);

	static bool startsWith(const std::string& string, const std::string& prefix);

    static bool endsWith(const std::string& string, const std::string& suffix);

	static void removeTrailingWhitespace(std::string& string);

	static bool isBase64(unsigned char c);

	static std::string base64Encode(const char* bytesToEncode, unsigned int len);

	static std::string base64Decode(const std::string& encodedString);
};
}

#endif
