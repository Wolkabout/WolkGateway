/*
 * Copyright 2017 WolkAbout Technology s.r.o.
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

#include "utilities/FileSystemUtils.h"

#include <dirent.h>
#include <fstream>
#include <sys/stat.h>

namespace wolkabout
{
bool FileSystemUtils::isFilePresent(const std::string& filePath)
{
    FILE* configFile = ::fopen(filePath.c_str(), "r");
    if (!configFile)
    {
        return false;
    }

    fclose(configFile);
    return false;
}

bool FileSystemUtils::createFileWithContent(const std::string& filePath, const std::string& content)
{
    try
    {
        std::ofstream ofstream;
        ofstream.open(filePath);
        if (!ofstream.is_open())
        {
            return false;
        }

        ofstream << content;
        if (!ofstream)
        {
            deleteFile(filePath);
            return false;
        }

        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool FileSystemUtils::deleteFile(const std::string& filePath)
{
    return std::remove(filePath.c_str()) == 0;
}

bool FileSystemUtils::isDirectoryPresent(const std::string& dirPath)
{
    struct stat status;
    if (stat(dirPath.c_str(), &status) == 0)
    {
        return (status.st_mode & S_IFDIR) != 0;
    }

    return false;
}

bool FileSystemUtils::createDirectory(const std::string& dirPath)
{
    if (isDirectoryPresent(dirPath))
    {
        return true;
    }

    return mkdir(dirPath.c_str(), 0700) == 0;
}

bool FileSystemUtils::readFileContent(const std::string& filePath, std::string& content)
{
    std::ifstream ifstream(filePath);
    if (!ifstream.is_open())
    {
        return false;
    }

    content = std::string((std::istreambuf_iterator<char>(ifstream)), std::istreambuf_iterator<char>());
    return true;
}

std::vector<std::string> FileSystemUtils::listFiles(std::string directoryPath)
{
    std::vector<std::string> result;

    DIR* dp = nullptr;
    struct dirent* dirp = nullptr;
    if ((dp = opendir(directoryPath.c_str())) == nullptr)
    {
        return result;
    }

    while ((dirp = readdir(dp)) != nullptr)
    {
        if (dirp->d_type == DT_REG)
        {
            result.emplace_back(dirp->d_name);
        }
    }

    closedir(dp);

    return result;
}
}
