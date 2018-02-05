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

#include "model/FirmwareUpdateCommand.h"

#include <string>
#include <utility>

namespace wolkabout
{
FirmwareUpdateCommand::FirmwareUpdateCommand() : m_type{FirmwareUpdateCommand::Type::UNKNOWN}, m_name{}, m_size{}, m_hash{},
	m_url{}, m_autoInstall{}
{
}

FirmwareUpdateCommand::FirmwareUpdateCommand(FirmwareUpdateCommand::Type type) : m_type{type}, m_name{}, m_size{}, m_hash{},
	m_url{}, m_autoInstall{}
{
}

FirmwareUpdateCommand::FirmwareUpdateCommand(FirmwareUpdateCommand::Type type, std::string name, uint_fast64_t size,
											 std::string hash, bool autoInstall) :
	m_type{type}, m_name{name}, m_size{size}, m_hash{hash}, m_url{}, m_autoInstall{autoInstall}
{
}

FirmwareUpdateCommand::FirmwareUpdateCommand(FirmwareUpdateCommand::Type type, std::string url, bool autoInstall) :
	m_type{type}, m_name{}, m_size{}, m_hash{}, m_url{url}, m_autoInstall{autoInstall}
{
}

const WolkOptional<std::string>& FirmwareUpdateCommand::getName() const
{
	return m_name;
}

const WolkOptional<uint_fast64_t>& FirmwareUpdateCommand::getSize() const
{
	return m_size;
}

const WolkOptional<std::string>& FirmwareUpdateCommand::getHash() const
{
	return m_hash;
}

const WolkOptional<std::string>& FirmwareUpdateCommand::getUrl() const
{
	return m_url;
}

const WolkOptional<bool>& FirmwareUpdateCommand::getAutoInstall() const
{
	return m_autoInstall;
}

FirmwareUpdateCommand::Type FirmwareUpdateCommand::getType() const
{
	return m_type;
}

}
