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

private:
    std::string m_name;
    AlarmSeverity m_severity;
    std::string m_reference;
    std::string m_message;
    std::string m_description;
};
}    // namespace wolkabout

#endif    // ALARMMANIFEST_H
