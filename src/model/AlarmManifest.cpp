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
}    // namespace wolkabout
