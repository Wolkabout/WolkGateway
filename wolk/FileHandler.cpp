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

#include "core/model/messages/FileBinaryResponseMessage.h"
#include "core/utilities/FileSystemUtils.h"

namespace wolkabout
{
FileHandler::FileHandler() : m_currentPacketData{}, m_previousPacketHash{} {}

void FileHandler::clear()
{
    m_currentPacketData = {};
    m_previousPacketHash = {};
}

FileHandler::StatusCode FileHandler::handle(const FileBinaryResponseMessage& response)
{
    const ByteArray hash = ByteUtils::hashSHA256(response.getData());
    if (hash != ByteUtils::toByteArray(response.getPreviousHash()))
    {
        return FileHandler::StatusCode::PACKAGE_HASH_NOT_VALID;
    }

    const auto validationHash =
      m_previousPacketHash.empty() ? ByteArray(ByteUtils::SHA_256_HASH_BYTE_LENGTH, 0) : m_previousPacketHash;

    if (validationHash != ByteUtils::toByteArray(response.getPreviousHash()))
    {
        return FileHandler::StatusCode::PREVIOUS_PACKAGE_HASH_NOT_VALID;
    }

    m_currentPacketData.insert(m_currentPacketData.end(), response.getData().begin(), response.getData().end());
    m_previousPacketHash = ByteUtils::toByteArray(response.getPreviousHash());

    return FileHandler::StatusCode::OK;
}

FileHandler::StatusCode FileHandler::validateFile(const ByteArray& fileHash) const
{
    if (fileHash == ByteUtils::hashSHA256(m_currentPacketData))
    {
        return FileHandler::StatusCode::OK;
    }

    return FileHandler::StatusCode::FILE_HASH_NOT_VALID;
}

FileHandler::StatusCode FileHandler::saveFile(const std::string& filePath) const
{
    if (FileSystemUtils::createBinaryFileWithContent(filePath, m_currentPacketData))
    {
        return FileHandler::StatusCode::OK;
    }

    return FileHandler::StatusCode::FILE_HANDLING_ERROR;
}

FileHandler::StatusCode FileHandler::saveFile(const std::string& fileName, const std::string& directory) const
{
    const std::string path = FileSystemUtils::composePath(fileName, directory);

    return saveFile(path);
}

}    // namespace wolkabout
