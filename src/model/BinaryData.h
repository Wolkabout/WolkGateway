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

#ifndef BINARYDATA_H
#define BINARYDATA_H

#include "utilities/ByteUtils.h"

namespace wolkabout
{
class BinaryData
{
public:
    BinaryData();

    /**
     * @brief BinaryData Constructs the object using binary data
     * Throws std::invalid_argument if the size of the binary data
     * is not big enough to contain valid data
     * @param value Binary data
     */
    BinaryData(const ByteArray& data);

    /**
     * @brief getData
     * @return data part of the binary packet
     */
    const ByteArray& getData() const;

    /**
     * @brief getHash
     * @return hash part of the binary packet
     */
    const ByteArray& getHash() const;

    /**
     * @brief valid Validates the packet using its hash
     * @return true if packet is valid, false otherwise
     */
    bool valid() const;

    /**
     * @brief validatePrevious Validates that param matches the previous hash
     * part of the binary packet
     * @param previousHash Previous hash to validate against
     * @return true if previous hash is valid, false otherwise
     */
    bool validatePrevious(const ByteArray& previousHash) const;

    /**
     * @brief validatePrevious Validates that the previous hash part of the
     * binary packet matches hash of an empty string
     * Used when packet is first in order and no previous hash exists
     * @return true if previous hash is valid, false otherwise
     */
    bool validatePrevious() const;

private:
    ByteArray m_value;

    ByteArray m_data;
    ByteArray m_hash;
    ByteArray m_previousHash;
};
}    // namespace wolkabout

#endif    // BINARYDATA_H
