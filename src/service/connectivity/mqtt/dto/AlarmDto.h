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

#ifndef ALARMDTO_H
#define ALARMDTO_H

#include "model/Alarm.h"

#include <string>

namespace wolkabout
{
class AlarmDto
{
public:
    AlarmDto() = default;
    AlarmDto(const Alarm& alarm);
    AlarmDto(unsigned long long rtc, std::string value);

    virtual ~AlarmDto() = default;

    unsigned long long int getRtc() const;
    const std::string& getValue() const;

private:
    unsigned long long int m_rtc;
    std::string m_value;
};
}

#endif
