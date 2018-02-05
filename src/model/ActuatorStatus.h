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

#ifndef ACTUATORSTATUS_H
#define ACTUATORSTATUS_H

#include "model/Reading.h"

#include <string>

namespace wolkabout
{
class ActuatorStatus : public Reading
{
public:
    enum class State
    {
        READY,
        BUSY,
        ERROR
    };

    ActuatorStatus();
    ActuatorStatus(std::string value, ActuatorStatus::State state);
    ActuatorStatus(std::string value, std::string reference, ActuatorStatus::State state);

    virtual ~ActuatorStatus() = default;

    ActuatorStatus::State getState() const;

    void acceptVisit(ReadingVisitor& visitor) override;

private:
    ActuatorStatus::State m_state;
};
}

#endif
