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

#ifndef READING_H
#define READING_H

#include <string>

namespace wolkabout
{
class ActuatorStatus;
class Alarm;
class SensorReading;

class ReadingVisitor
{
public:
    ReadingVisitor() = default;
    virtual ~ReadingVisitor() = default;

    virtual void visit(ActuatorStatus& actuatorStatus) = 0;
    virtual void visit(Alarm& alarm) = 0;
    virtual void visit(SensorReading& sensorReading) = 0;
};

class Reading
{
public:
    Reading(std::string value, std::string reference, unsigned long long int rtc = 0);

    virtual ~Reading() = default;

    const std::string& getValue() const;
    const std::string& getReference() const;
    unsigned long long int getRtc() const;

    virtual void acceptVisit(ReadingVisitor& visitor) = 0;

private:
    std::string m_value;
    std::string m_reference;
    unsigned long long int m_rtc;
};
}

#endif
