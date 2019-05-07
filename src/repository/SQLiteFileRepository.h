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

#ifndef SQLITEFILEREPOSITORY_H
#define SQLITEFILEREPOSITORY_H

#include "repository/FileRepository.h"

#include <mutex>

namespace Poco
{
namespace Data
{
    class Session;
}
}    // namespace Poco

namespace wolkabout
{
class SQLiteFileRepository : public FileRepository
{
public:
    explicit SQLiteFileRepository(const std::string& connectionString);

    std::unique_ptr<FileInfo> getFileInfo(const std::string& fileName) override;
    std::unique_ptr<std::vector<std::string>> getAllFileNames() override;

    void store(const FileInfo& info) override;

    void remove(const std::string& fileName) override;
    void removeAll() override;

    bool containsInfoForFile(const std::string& fileName) override;

private:
    void update(const FileInfo& info);

    std::recursive_mutex m_mutex;
    std::unique_ptr<Poco::Data::Session> m_session;

    static const std::string FILE_INFO_TABLE;
    static const std::string ID_COLUMN;
    static const std::string NAME_COLUMN;
    static const std::string HASH_COLUMN;
    static const std::string PATH_COLUMN;
};
}    // namespace wolkabout

#endif    // SQLITEFILEREPOSITORY_H
