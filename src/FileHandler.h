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

#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include "utilities/ByteUtils.h"
#include <string>

namespace wolkabout
{
class BinaryData;

class FileHandler
{
public:
	enum class StatusCode
	{
		OK = 0,
		PACKAGE_HASH_NOT_VALID,
		PREVIOUS_PACKAGE_HASH_NOT_VALID,
		FILE_HASH_NOT_VALID,
		FILE_HANDLING_ERROR
	};

	FileHandler();

	virtual ~FileHandler() = default;

	void clear();

	FileHandler::StatusCode handleData(const BinaryData& binaryData);

	FileHandler::StatusCode validateFile(const ByteArray& fileHash) const;

	FileHandler::StatusCode saveFile(const std::string& filePath) const;

private:
	ByteArray m_currentPacketData;
	ByteArray m_previousPacketHash;
};
}


#endif // FILEHANDLER_H
