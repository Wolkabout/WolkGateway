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

#ifndef FILEINFO_H
#define FILEINFO_H

#include <string>

namespace wolkabout
{
struct FileInfo
{
    FileInfo(std::string name_, std::string path_, std::string hash_)
    : name{std::move(name_)}, path{std::move(path_)}, hash{std::move(hash_)}
    {
    }

    std::string name;
    std::string path;
    std::string hash;
};
}    // namespace wolkabout

#endif    // FILEINFO_H
