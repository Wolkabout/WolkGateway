#ifndef ACTUATORMANIFEST_H
#define ACTUATORMANIFEST_H

#include <initializer_list>
#include <string>
#include <vector>

namespace wolkabout
{
class ActuatorManifest
{
public:
    enum class DataType
    {
        STRING,
        NUMERIC,
        BOOLEAN
    };

    ActuatorManifest() = default;

    ActuatorManifest(std::string name, std::string reference, std::string description, std::string unit,
                     std::string readingType, ActuatorManifest::DataType dataType, unsigned int precision,
                     double minimum, double maximum);

    ActuatorManifest(std::string name, std::string reference, std::string description, std::string unit,
                     std::string readingType, ActuatorManifest::DataType dataType, unsigned int precision,
                     double minimum, double maximum, std::string delimiter, std::vector<std::string> labels);

    virtual ~ActuatorManifest() = default;

    const std::string& getName() const;
    ActuatorManifest& setName(const std::string& name);

    const std::string& getReference() const;
    ActuatorManifest& setReference(const std::string& reference);

    const std::string& getDescription() const;
    ActuatorManifest& setDescription(const std::string description);

    const std::string& getUnit() const;
    ActuatorManifest& setUnit(const std::string& unit);

    const std::string& getReadingType() const;    // TODO: @N. Antic
    ActuatorManifest& setReadingType(const std::string& readingType);

    ActuatorManifest::DataType getDataType() const;
    ActuatorManifest& setDataType(ActuatorManifest::DataType dataType);

    unsigned int getPrecision() const;    // TODO: @N. Antic
    ActuatorManifest& setPrecision(unsigned int precision);

    double getMinimum() const;
    ActuatorManifest& setMinimum(double minimum);

    double getMaximum() const;
    ActuatorManifest& setMaximum(double maximum);

    const std::string& getDelimiter() const;
    ActuatorManifest& setDelimiter(const std::string& delimited);

    const std::vector<std::string>& getLabels() const;
    ActuatorManifest& setLabels(std::initializer_list<std::string> labels);

private:
    std::string m_name;
    std::string m_reference;
    std::string m_description;
    std::string m_unit;
    std::string m_readingType;
    DataType m_dataType;
    unsigned int m_precision;

    double m_minimum;
    double m_maximum;

    std::string m_delimiter;
    std::vector<std::string> m_labels;
};
}    // namespace wolkabout

#endif    // ACTUATORMANIFEST_H
