/*
 * Copyright 2019 WolkAbout Technology s.r.o.
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

#ifndef FILEREPOSITORY_H
#define FILEREPOSITORY_H

#include "FileInfo.h"

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class FileRepository
{
public:
    virtual ~FileRepository() = default;

    virtual std::unique_ptr<FileInfo> getFileInfo(const std::string& fileName) = 0;
    virtual std::unique_ptr<std::vector<std::string>> getAllFileNames() = 0;

    virtual void store(const FileInfo& info) = 0;

    virtual void remove(const std::string& fileName) = 0;
    virtual void removeAll() = 0;

    virtual bool containsInfoForFile(const std::string& fileName) = 0;
};
}    // namespace wolkabout

#endif    // FILEREPOSITORY_H
