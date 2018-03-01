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

#ifndef ALARMMANIFEST_H
#define ALARMMANIFEST_H

#include <string>

namespace wolkabout
{
class AlarmManifest
{
public:
    enum class AlarmSeverity
    {
        ALERT,
        ERROR,
        CRITICAL
    };

    AlarmManifest() = default;
    AlarmManifest(std::string name, AlarmManifest::AlarmSeverity severity, std::string reference, std::string message,
                  std::string description);

    virtual ~AlarmManifest() = default;

    const std::string& getName() const;
    AlarmManifest& setName(const std::string& name);

    AlarmManifest::AlarmSeverity getSeverity() const;
    AlarmManifest& setSeverity(AlarmManifest::AlarmSeverity severity);

    const std::string& getReference() const;
    AlarmManifest& setReference(const std::string& reference);

    const std::string& getMessage() const;
    AlarmManifest& setMessage(const std::string& message);

    const std::string& getDescription() const;
    AlarmManifest& setDescription(const std::string& description);

    bool operator==(AlarmManifest& rhs) const;
    bool operator!=(AlarmManifest& rhs) const;

private:
    std::string m_name;
    AlarmSeverity m_severity;
    std::string m_reference;
    std::string m_message;
    std::string m_description;
};
}    // namespace wolkabout

#endif    // ALARMMANIFEST_H
