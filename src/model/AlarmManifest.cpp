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

#include "model/AlarmManifest.h"

#include <string>
#include <utility>

namespace wolkabout
{
AlarmManifest::AlarmManifest(std::string name, AlarmManifest::AlarmSeverity severity, std::string reference,
                             std::string message, std::string description)
: m_name(std::move(name))
, m_severity(severity)
, m_reference(std::move(reference))
, m_message(std::move(message))
, m_description(std::move(description))
{
}

const std::string& AlarmManifest::getName() const
{
    return m_name;
}

AlarmManifest& AlarmManifest::setName(const std::string& name)
{
    m_name = name;
    return *this;
}

AlarmManifest::AlarmSeverity AlarmManifest::getSeverity() const
{
    return m_severity;
}

AlarmManifest& AlarmManifest::setSeverity(AlarmManifest::AlarmSeverity severity)
{
    m_severity = severity;
    return *this;
}

const std::string& AlarmManifest::getReference() const
{
    return m_reference;
}

AlarmManifest& AlarmManifest::setReference(const std::string& reference)
{
    m_reference = reference;
    return *this;
}

const std::string& AlarmManifest::getMessage() const
{
    return m_message;
}

AlarmManifest& AlarmManifest::setMessage(const std::string& message)
{
    m_message = message;
    return *this;
}

const std::string& AlarmManifest::getDescription() const
{
    return m_description;
}

AlarmManifest& AlarmManifest::setDescription(const std::string& description)
{
    m_description = description;
    return *this;
}

bool AlarmManifest::operator==(AlarmManifest& rhs) const
{
    if (m_name != rhs.m_name || m_severity != rhs.m_severity || m_reference != rhs.m_reference ||
        m_message != rhs.m_message || m_description != rhs.m_description)
    {
        return false;
    }

    return true;
}

bool AlarmManifest::operator!=(AlarmManifest& rhs) const
{
    return !(*this == rhs);
}
}    // namespace wolkabout
