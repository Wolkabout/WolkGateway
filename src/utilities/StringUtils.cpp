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

#include "utilities/StringUtils.h"

#include <string>
#include <vector>

namespace wolkabout
{
static const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
										"abcdefghijklmnopqrstuvwxyz"
										"0123456789+/";

bool StringUtils::contains(const std::string& string, char c)
{
    return string.find(c) != std::string::npos;
}

bool StringUtils::contains(const std::string& string, const std::string& substring)
{
    return string.find(substring) != std::string::npos;
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

bool StringUtils::startsWith(const std::string& string, const std::string& prefix)
{
	return string.size() >= prefix.size() && 0 == string.compare(0, prefix.size(), prefix);
}

bool StringUtils::endsWith(const std::string& string, const std::string& suffix)
{
    return string.size() >= suffix.size() && 0 == string.compare(string.size() - suffix.size(), suffix.size(), suffix);
}

void StringUtils::removeTrailingWhitespace(std::string& string)
{
	while(!string.empty() && std::isspace(*string.rbegin()))
	{
		string.erase(string.length() - 1);
	}
}

bool StringUtils::isBase64(unsigned char c)
{
	return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string StringUtils::base64Encode(const char* bytesToEncode, unsigned int len)
{
	std::string ret;
	int i = 0;
	int j = 0;
	unsigned char char_array_3[3];
	unsigned char char_array_4[4];

	while (len--)
	{
		char_array_3[i++] = *(bytesToEncode++);
		if (i == 3)
		{
			char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
			char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
			char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
			char_array_4[3] = char_array_3[2] & 0x3f;

			for (i = 0; (i < 4); i++)
				ret += base64_chars[char_array_4[i]];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 3; j++)
			char_array_3[j] = '\0';

		char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
		char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
		char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
		char_array_4[3] = char_array_3[2] & 0x3f;

		for (j = 0; (j < i + 1); j++)
			ret += base64_chars[char_array_4[j]];

		while ((i++ < 3))
			ret += '=';
	}

	return ret;
}

std::string StringUtils::base64Decode(const std::string& encodedString)
{
	std::string ret;

	int in_len = encodedString.size();
	int i = 0;
	int j = 0;
	int in_ = 0;
	unsigned char char_array_4[4], char_array_3[3];

	while (in_len-- && (encodedString[in_] != '=') && isBase64(encodedString[in_]))
	{
		char_array_4[i++] = encodedString[in_];
		in_++;
		if (i == 4)
		{
			for (i = 0; i < 4; i++)
				char_array_4[i] = base64_chars.find(char_array_4[i]);

			char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
			char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
			char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

			for (i = 0; (i < 3); i++)
				ret += char_array_3[i];
			i = 0;
		}
	}

	if (i)
	{
		for (j = i; j < 4; j++)
			char_array_4[j] = 0;

		for (j = 0; j < 4; j++)
			char_array_4[j] = base64_chars.find(char_array_4[j]);

		char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
		char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
		char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

		for (j = 0; (j < i - 1); j++)
			ret += char_array_3[j];
	}

	return ret;
}
}
