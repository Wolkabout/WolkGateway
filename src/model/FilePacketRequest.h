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

#ifndef FILEPACKETREQUEST_H
#define FILEPACKETREQUEST_H

#include <cstdint>
#include <string>

namespace wolkabout
{
class FilePacketRequest
{
public:
    FilePacketRequest();
    FilePacketRequest(const std::string& fileName, unsigned chunkIndex, std::uint_fast64_t chunkSize);

    const std::string& getFileName() const;
    unsigned getChunkIndex() const;
    std::uint_fast64_t getChunkSize() const;

private:
    const std::string m_fileName;
    const unsigned m_chunkIndex;
    const uint_fast64_t m_chunkSize;
};
}    // namespace wolkabout

#endif    // FILEPACKETREQUEST_H
