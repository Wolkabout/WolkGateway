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

#ifndef URLFILEDOWNLOADER_H
#define URLFILEDOWNLOADER_H

#include <functional>
#include <string>

namespace wolkabout
{
class UrlFileDownloader
{
public:
    enum class Error
    {
        UNSPECIFIED_ERROR,
        FILE_SYSTEM_ERROR,
        MALFORMED_URL,
        UNSUPPORTED_FILE_SIZE
    };

    virtual ~UrlFileDownloader() = default;

    /**
     * @brief download Starts downloading of the file from provided url
     * Should be implemented in a thread safe manner
     * Should support parallel downloads
     * @param url Url of the file to download
     * @param downloadDirectory Directory where to download file
     * @param onSuccessCallback Function to call when file is downloaded with key and file path as argument
     * @param onFailCallback Function to call when download fails with key and error code as argument
     */
    virtual void download(
      const std::string& url, const std::string& downloadDirectory,
      std::function<void(const std::string& url, const std::string& filePath)> onSuccessCallback,
      std::function<void(const std::string& url, UrlFileDownloader::Error errorCode)> onFailCallback) = 0;
    /**
     * @brief abort Aborts file download and removes any saved data
     * Should be implemented in a thread safe manner
     * @param url Url of the file being downloaded
     */
    virtual void abort(const std::string& url) = 0;
};
}    // namespace wolkabout

#endif    // URLFILEDOWNLOADER_H
