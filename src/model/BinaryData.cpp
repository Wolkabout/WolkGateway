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

#include "BinaryData.h"
#include <stdexcept>

namespace wolkabout
{

BinaryData::BinaryData() : m_value{}, m_data{}, m_hash{}, m_previousHash{}
{
}

BinaryData::BinaryData(const ByteArray& value) : m_value{value}, m_data{}, m_hash{}, m_previousHash{}
{
	if(value.size() <= 2 * ByteUtils::SHA_256_HASH_BYTE_LENGTH)
	{
		throw std::invalid_argument("Binary data size is smaller than required to fit standard data packet");
	}

	m_previousHash = {m_value.begin(), m_value.begin() + ByteUtils::SHA_256_HASH_BYTE_LENGTH};
	m_data = {m_value.begin() + ByteUtils::SHA_256_HASH_BYTE_LENGTH, m_value.end() - ByteUtils::SHA_256_HASH_BYTE_LENGTH};
	m_hash = {m_value.end() - ByteUtils::SHA_256_HASH_BYTE_LENGTH, m_value.end()};
}

const ByteArray& BinaryData::getData() const
{
	return m_data;
}

const ByteArray& BinaryData::getHash() const
{
	return m_hash;
}

bool BinaryData::valid() const
{
	const ByteArray hash = ByteUtils::hashSHA256(m_data);

	return hash == m_hash;
}

bool BinaryData::validatePrevious() const
{
	// validate with all zero hash
	return validatePrevious(ByteArray(ByteUtils::SHA_256_HASH_BYTE_LENGTH, 0));
}

bool BinaryData::validatePrevious(const ByteArray& previousHash) const
{
	return m_previousHash == previousHash;
}

}
