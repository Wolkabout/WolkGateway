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

#include "ActuatorStatus.h"
#include "Reading.h"

#include <string>
#include <utility>

namespace wolkabout
{
ActuatorStatus::ActuatorStatus() : Reading("", ""), m_state(ActuatorStatus::State::READY) {}

ActuatorStatus::ActuatorStatus(std::string value, ActuatorStatus::State state)
: Reading(std::move(value), ""), m_state(std::move(state))
{
}

ActuatorStatus::ActuatorStatus(std::string value, std::string reference, ActuatorStatus::State state)
: Reading(std::move(value), std::move(reference)), m_state(std::move(state))
{
}

ActuatorStatus::State ActuatorStatus::getState() const
{
    return m_state;
}

void ActuatorStatus::acceptVisit(ReadingVisitor& visitor)
{
    visitor.visit(*this);
}
}
