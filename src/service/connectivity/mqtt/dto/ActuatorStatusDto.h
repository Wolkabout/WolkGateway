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

#ifndef ACTUATORSTATUSDTO_H
#define ACTUATORSTATUSDTO_H

#include "model/ActuatorStatus.h"

#include <string>

namespace wolkabout
{
class ActuatorStatusDto
{
public:
    ActuatorStatusDto() = default;
    ActuatorStatusDto(ActuatorStatus actuatorStatus);
    ActuatorStatusDto(ActuatorStatus::State state, std::string value);

    virtual ~ActuatorStatusDto() = default;

    ActuatorStatus::State getState() const;
    const std::string& getValue() const;

private:
    ActuatorStatus::State m_state;
    std::string m_value;
};
}

#endif
