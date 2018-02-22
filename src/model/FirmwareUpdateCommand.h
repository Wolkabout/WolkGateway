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

#ifndef FIRMWAREUPDATECOMMAND_H
#define FIRMWAREUPDATECOMMAND_H

#include "WolkOptional.h"
#include <cstdint>
#include <string>

namespace wolkabout
{
class FirmwareUpdateCommand
{
public:
    enum class Type
    {
        FILE_UPLOAD,
        URL_DOWNLOAD,
        INSTALL,
        ABORT,
        UNKNOWN = -1
    };

    FirmwareUpdateCommand();

    explicit FirmwareUpdateCommand(FirmwareUpdateCommand::Type type);

    FirmwareUpdateCommand(FirmwareUpdateCommand::Type type, std::string name, std::uint_fast64_t size, std::string hash,
                          bool autoInstall);

    FirmwareUpdateCommand(FirmwareUpdateCommand::Type type, std::string url, bool autoInstall);

    FirmwareUpdateCommand::Type getType() const;

    const WolkOptional<std::string>& getName() const;
    const WolkOptional<std::uint_fast64_t>& getSize() const;
    const WolkOptional<std::string>& getHash() const;

    const WolkOptional<std::string>& getUrl() const;

    const WolkOptional<bool>& getAutoInstall() const;

private:
    FirmwareUpdateCommand::Type m_type;

    WolkOptional<std::string> m_name;
    WolkOptional<std::uint_fast64_t> m_size;
    WolkOptional<std::string> m_hash;

    WolkOptional<std::string> m_url;

    WolkOptional<bool> m_autoInstall;
};
}    // namespace wolkabout

#endif
