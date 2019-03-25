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

#include "repository/SQLiteFileRepository.h"
#include "utilities/Logger.h"

#include <Poco/Data/Binding.h>
#include <Poco/Data/SQLite/Connector.h>
#include <Poco/Data/SQLite/SQLiteException.h>
#include <Poco/Data/Session.h>
#include <Poco/Data/Statement.h>

using namespace Poco::Data::Keywords;
using Poco::Data::Statement;

namespace wolkabout
{
const std::string SQLiteFileRepository::FILE_INFO_TABLE = "file_info";
const std::string SQLiteFileRepository::ID_COLUMN = "id";
const std::string SQLiteFileRepository::NAME_COLUMN = "name";
const std::string SQLiteFileRepository::HASH_COLUMN = "hash";
const std::string SQLiteFileRepository::PATH_COLUMN = "path";

SQLiteFileRepository::SQLiteFileRepository(const std::string& connectionString)
{
    Poco::Data::SQLite::Connector::registerConnector();
    m_session = std::unique_ptr<Poco::Data::Session>(
      new Poco::Data::Session(Poco::Data::SQLite::Connector::KEY, connectionString));
    Statement statement(*m_session);

    statement << "CREATE TABLE IF NOT EXISTS " << FILE_INFO_TABLE << " (" << ID_COLUMN
              << " INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT, " << NAME_COLUMN << " TEXT NOT NULL UNIQUE, "
              << HASH_COLUMN << " TEXT NOT NULL, " << PATH_COLUMN << " TEXT NOT NULL UNIQUE);";

    statement << "PRAGMA foreign_keys=on;";

    statement.execute();
}

std::unique_ptr<FileInfo> SQLiteFileRepository::getFileInfo(const std::string& fileName)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    try
    {
        std::string hash, path;

        Statement statement(*m_session);
        statement << "SELECT " << HASH_COLUMN << ", " << PATH_COLUMN << " FROM " << FILE_INFO_TABLE << " WHERE "
                  << FILE_INFO_TABLE << "." << NAME_COLUMN << "=?;",
          useRef(fileName), into(hash), into(path);
        if (statement.execute() == 0)
        {
            return nullptr;
        }

        return std::unique_ptr<FileInfo>(new FileInfo{fileName, hash, path});
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteFileRepository: Error finding file info for file " << fileName;
        return nullptr;
    }
}

std::unique_ptr<std::vector<std::string>> SQLiteFileRepository::getAllFileNames()
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    auto fileNames = std::unique_ptr<std::vector<std::string>>(new std::vector<std::string>());

    try
    {
        Statement statement(*m_session);
        statement << "SELECT " << NAME_COLUMN << " FROM " << FILE_INFO_TABLE << ";", into(*fileNames), now;
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteFileRepository: Error finding file names";
    }

    return fileNames;
}

void SQLiteFileRepository::store(const FileInfo& info)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    if (containsInfoForFile(info.name))
    {
        update(info);
        return;
    }

    try
    {
        Statement statement(*m_session);
        statement << "INSERT INTO " << FILE_INFO_TABLE << " (" << NAME_COLUMN << ", " << HASH_COLUMN << ", "
                  << PATH_COLUMN << ")"
                  << " VALUES(?, ?, ?);",
          useRef(info.name), useRef(info.hash), useRef(info.path);

        statement << "COMMIT;", now;
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteFileRepository: Error saving file info for file " << info.name;
    }
}

void SQLiteFileRepository::remove(const std::string& fileName)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    if (!containsInfoForFile(fileName))
    {
        return;
    }

    try
    {
        Statement statement(*m_session);
        statement << "DELETE FROM " << FILE_INFO_TABLE << " WHERE " << FILE_INFO_TABLE << "." << NAME_COLUMN << "=?;",
          useRef(fileName), now;
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteFileRepository: Error removing file info for file " << fileName;
    }
}

void SQLiteFileRepository::removeAll()
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    try
    {
        Statement statement(*m_session);
        statement << "DELETE FROM " << FILE_INFO_TABLE << ";", now;
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteFileRepository: Error removing all file info";
    }
}

bool SQLiteFileRepository::containsInfoForFile(const std::string& fileName)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    try
    {
        Poco::UInt64 count;
        Statement statement(*m_session);
        statement << "SELECT COUNT(*) FROM " << FILE_INFO_TABLE << " WHERE " << FILE_INFO_TABLE << "." << NAME_COLUMN
                  << "=?;",
          useRef(fileName), into(count), now;

        return count != 0;
    }
    catch (...)
    {
        LOG(ERROR) << "SQLiteFileRepository: Error finding file info for file " << fileName;
        return false;
    }
}

void SQLiteFileRepository::update(const FileInfo& info)
{
    std::lock_guard<decltype(m_mutex)> l(m_mutex);

    remove(info.name);
    store(info);
}
}    // namespace wolkabout