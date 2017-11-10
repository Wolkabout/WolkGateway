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

#ifndef ACTUATORCOMMAND_H
#define ACTUATORCOMMAND_H

#include <string>

namespace wolkabout
{
class ActuatorCommand
{
public:
    enum class Type
    {
        SET,
        STATUS
    };

    ActuatorCommand();
    ActuatorCommand(ActuatorCommand::Type type, std::string reference, std::string value);

    virtual ~ActuatorCommand() = default;

    ActuatorCommand::Type getType() const;
    const std::string& getReference() const;
    const std::string& getValue() const;

private:
    ActuatorCommand::Type m_type;
    std::string m_reference;
    std::string m_value;
};
}

#endif
