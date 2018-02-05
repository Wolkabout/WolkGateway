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

#include "FileHandler.h"
#include "model/BinaryData.h"
#include "utilities/FileSystemUtils.h"

namespace wolkabout
{

FileHandler::FileHandler() :
	m_currentPacketData{},
	m_previousPacketHash{}
{
}

void FileHandler::clear()
{
	m_currentPacketData = {};
	m_previousPacketHash = {};
}

FileHandler::StatusCode FileHandler::handleData(const BinaryData& binaryData)
{
	if(!binaryData.valid())
	{
		return FileHandler::StatusCode::PACKAGE_HASH_NOT_VALID;
	}

	if(m_previousPacketHash.empty())
	{
		if(!binaryData.validatePrevious())
		{
			return FileHandler::StatusCode::PREVIOUS_PACKAGE_HASH_NOT_VALID;
		}
	}
	else
	{
		if(!binaryData.validatePrevious(m_previousPacketHash))
		{
			return FileHandler::StatusCode::PREVIOUS_PACKAGE_HASH_NOT_VALID;
		}
	}

	m_currentPacketData.insert(m_currentPacketData.end(), binaryData.getData().begin(), binaryData.getData().end());
	m_previousPacketHash = binaryData.getHash();

	return FileHandler::StatusCode::OK;
}

FileHandler::StatusCode FileHandler::validateFile(const ByteArray& fileHash) const
{
	if(fileHash == ByteUtils::hashSHA256(m_currentPacketData))
	{
		return FileHandler::StatusCode::OK;
	}

	return FileHandler::StatusCode::FILE_HASH_NOT_VALID;
}

FileHandler::StatusCode FileHandler::saveFile(const std::string& filePath) const
{
	if(FileSystemUtils::createBinaryFileWithContent(filePath, m_currentPacketData))
	{
		return FileHandler::StatusCode::OK;
	}

	return FileHandler::StatusCode::FILE_HANDLING_ERROR;
}

}
