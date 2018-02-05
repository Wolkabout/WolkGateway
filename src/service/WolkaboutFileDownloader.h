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

#ifndef WOLKABOUTFILEDOWNLOADER_H
#define WOLKABOUTFILEDOWNLOADER_H

#include "utilities/ByteUtils.h"
#include <string>
#include <functional>
#include <cstdint>

namespace wolkabout
{
class WolkaboutFileDownloader
{
public:
	enum class Error
	{
		UNSPECIFIED_ERROR,
		FILE_SYSTEM_ERROR,
		RETRY_COUNT_EXCEEDED,
		UNSUPPORTED_FILE_SIZE
	};

	virtual ~WolkaboutFileDownloader() = default;

	/**
	 * @brief download Starts downloading of the file from Wolkabout platform
	 * Should be implemented in a thread safe manner
	 * @param fileName Name of the file to be downloaded
	 * @param fileSize Size of the file to be downloaded
	 * @param fileHash SHA256 hash of the file to be downloaded
	 * @param downloadDirectory Directory where to download file
	 * @param onSuccessCallback Function to call when file is downloaded with file path as argument
	 * @param onFailCallback Function to call when download fails with error code as argument
	 */
	virtual void download(const std::string& fileName, std::uint_fast64_t fileSize, const ByteArray& fileHash,
						  const std::string& downloadDirectory,
						  std::function<void(const std::string& filePath)> onSuccessCallback,
						  std::function<void(WolkaboutFileDownloader::Error errorCode)> onFailCallback) = 0;

	/**
	 * @brief abort Aborts file download and removes any saved data
	 * Should be implemented in a thread safe manner
	 */
	virtual void abort() = 0;
};
}

#endif // WOLKABOUTFILEDOWNLOADER_H
