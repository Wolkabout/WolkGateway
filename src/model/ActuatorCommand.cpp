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

#include "model/ActuatorCommand.h"

#include <string>
#include <utility>

namespace wolkabout
{
ActuatorCommand::ActuatorCommand() : m_type(ActuatorCommand::Type::STATUS), m_reference(""), m_value("") {}

ActuatorCommand::ActuatorCommand(wolkabout::ActuatorCommand::Type type, std::string reference, std::string value)
: m_type(type), m_reference(std::move(reference)), m_value(std::move(value))
{
}

ActuatorCommand::Type ActuatorCommand::getType() const
{
    return m_type;
}

const std::string& ActuatorCommand::getReference() const
{
    return m_reference;
}

const std::string& ActuatorCommand::getValue() const
{
    return m_value;
}
}
