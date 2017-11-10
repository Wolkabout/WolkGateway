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

#ifndef SENSORREADINGDTO_H
#define SENSORREADINGDTO_H

#include "model/SensorReading.h"

#include <string>

namespace wolkabout
{
class SensorReadingDto
{
public:
    SensorReadingDto() = default;
    SensorReadingDto(const SensorReading& sensorReading);
    SensorReadingDto(unsigned long long int rtc, std::string value);

    virtual ~SensorReadingDto() = default;

    unsigned long long int getRtc() const;
    const std::string& getValue() const;

private:
    unsigned long long int m_rtc;
    std::string m_value;
};
}

#endif
