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

#ifndef FILELISTENER_H
#define FILELISTENER_H

#include <functional>
#include <string>

namespace wolkabout
{
/**
 * This class presents an interface for an object that is interested in ongoing of the `FileDownloadService`, and it
 * will be notified of new files that are downloaded/deleted.
 */
class FileListener
{
public:
    /**
     * The default overridable destructor.
     */
    virtual ~FileListener() = default;

    /**
     * This is the method that is invoked by the service to notify of the directory that is being used to store files,
     * and in which they are all accessible.
     *
     * @param absolutePath The absolute path of the directory where files are stored.
     */
    virtual void receiveDirectory(const std::string& absolutePath) = 0;

    /**
     * This is the method that is invoked by the service to give a lambda expression that can be invoked by the listener
     * to create a file if they wish to do so.
     */
    virtual void setCreateFileLambda(
      std::function<void(const std::string&, const std::string&, const std::string&)> fileCreationLambda) = 0;

    /**
     * This is the method that is invoked by the service to notify that a new file has been made available to download,
     * and we can say whether or not we want to download the file.
     *
     * @param fileName The name of the newly initiated file.
     * @return Whether or not we want to download the file.
     */
    virtual bool chooseToDownload(const std::string& fileName) = 0;

    /**
     * This is the method that is invoked by the service to notify that a new file has been downloaded and is placed in
     * the file system. From this point on, the service can manipulate the file further, and cause action based on that.
     *
     * @param fileName The name of the newly downloaded file.
     */
    virtual void onNewFile(const std::string& fileName) = 0;

    /**
     * This is the method that is invoked by the service to notify that a file has been deleted or purged with all the
     * other ones off the file system. From this point on, the service can no longer count on the file existing.
     *
     * @param fileName The name of the removed file.
     */
    virtual void onRemovedFile(const std::string& fileName) = 0;
};
}    // namespace wolkabout

#endif    // FILELISTENER_H
