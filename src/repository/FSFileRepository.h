/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#ifndef WOLKGATEWAY_FSFILEREPOSITORY_H
#define WOLKGATEWAY_FSFILEREPOSITORY_H

#include "FileRepository.h"

namespace wolkabout
{
class FSFileRepository : public FileRepository
{
public:
    /**
     * Default constructor for the FileSystemFileRepository.
     *
     * @param folderPath The path to the folder which stores the files.
     */
    explicit FSFileRepository(std::string folderPath);

    /**
     * This method serves to return the information about a file.
     * This is an overridden method from `FileRepository`.
     *
     * @param fileName The name of the file we wish to gather information about.
     * @return The file info if the file with the name is found.
     */
    std::unique_ptr<FileInfo> getFileInfo(const std::string& fileName) override;

    /**
     * This method serves to return a list of files that are found locally.
     * This is an overridden method from `FileRepository`.
     *
     * @return The list of names of files that are found locally.
     */
    std::unique_ptr<std::vector<std::string>> getAllFileNames() override;

    /**
     * This method serves to store information about a file.
     * This is an overridden method from `FileRepository`.
     *
     * @param info The info struct that needs to be stored about files.
     */
    void store(const FileInfo& info) override;

    /**
     * This method removes the specific file with the name.
     * This is an overridden method from `FileRepository`.
     *
     * @param fileName The name of the file the user wishes to be removed.
     */
    void remove(const std::string& fileName) override;

    /**
     * This method removes all files that are locally found.
     * This is an overridden method from `FileRepository`.
     */
    void removeAll() override;

    /**
     * This method returns whether or not we can obtain information about a file.
     * This is an overridden method from `FileRepository`.
     *
     * @param fileName The name of the file the user wishes to get information.
     * @return Whether or not we can obtain info about a file.
     */
    bool containsInfoForFile(const std::string& fileName) override;

private:
    /**
     * This is an internal method that composes the full path for a file, with the folder that it is in.
     *
     * @param fileName The name of the file.
     * @return The full path with the folder we are working with.
     */
    std::string composeFilePath(const std::string& fileName);

    /**
     * This is an internal method that explicitly obtains a SHA256 hash of a file.
     *
     * @param filePath The path to the file that we wish to find a SHA256 hash of.
     * @return The SHA256 of the file content.
     */
    static std::string calculateFileHash(const std::string& filePath);

    // This is where we store the path to the folder that is used for file management.
    std::string m_folderPath;
};
}    // namespace wolkabout

#endif    // WOLKGATEWAY_FSFILEREPOSITORY_H
