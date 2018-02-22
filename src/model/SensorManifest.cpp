#include "model/SensorManifest.h"

#include <initializer_list>
#include <string>
#include <vector>

namespace wolkabout
{
SensorManifest::SensorManifest(std::string name, std::string reference, std::string description, std::string unit,
                               std::string readingType, SensorManifest::DataType dataType, unsigned int precision,
                               double minimum, double maximum)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_description(std::move(description))
, m_unit(std::move(unit))
, m_readingType(std::move(readingType))
, m_dataType(dataType)
, m_precision(precision)
, m_minimum(minimum)
, m_maximum(maximum)
, m_delimiter("")
, m_labels({})
{
}

SensorManifest::SensorManifest(std::string name, std::string reference, std::string description, std::string unit,
                               std::string readingType, SensorManifest::DataType dataType, unsigned int precision,
                               double minimum, double maximum, std::string delimiter, std::vector<std::string> labels)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_description(std::move(description))
, m_unit(std::move(unit))
, m_readingType(std::move(readingType))
, m_dataType(dataType)
, m_precision(precision)
, m_minimum(minimum)
, m_maximum(maximum)
, m_delimiter(std::move(delimiter))
, m_labels(std::move(labels))
{
}

const std::string& SensorManifest::getName() const
{
    return m_name;
}

wolkabout::SensorManifest& SensorManifest::setName(const std::string& name)
{
    m_name = name;
    return *this;
}

const std::string& SensorManifest::getReference() const
{
    return m_reference;
}

SensorManifest& SensorManifest::setReference(const std::string& reference)
{
    m_reference = reference;
    return *this;
}

const std::string& SensorManifest::getDescription() const
{
    return m_description;
}

SensorManifest& SensorManifest::setDescription(const std::string description)
{
    m_description = description;
    return *this;
}

const std::string& SensorManifest::getUnit() const
{
    return m_unit;
}

SensorManifest& SensorManifest::setUnit(const std::string& unit)
{
    m_unit = unit;
    return *this;
}

const std::string& SensorManifest::getReadingType() const
{
    return m_readingType;
}

SensorManifest& SensorManifest::setReadingType(const std::string& readingType)
{
    m_readingType = readingType;
    return *this;
}

SensorManifest::DataType SensorManifest::getDataType() const
{
    return m_dataType;
}

SensorManifest& SensorManifest::setDataType(SensorManifest::DataType dataType)
{
    m_dataType = dataType;
    return *this;
}

unsigned int SensorManifest::getPrecision() const
{
    return m_precision;
}

SensorManifest& SensorManifest::setPrecision(unsigned int precision)
{
    m_precision = precision;
    return *this;
}

double SensorManifest::getMinimum() const
{
    return m_minimum;
}

SensorManifest& SensorManifest::setMinimum(double minimum)
{
    m_minimum = minimum;
    return *this;
}

double SensorManifest::getMaximum() const
{
    return m_maximum;
}

SensorManifest& SensorManifest::setMaximum(double maximum)
{
    m_maximum = maximum;
    return *this;
}

const std::string& SensorManifest::getDelimiter() const
{
    return m_delimiter;
}

SensorManifest& SensorManifest::setDelimiter(const std::string& delimited)
{
    m_delimiter = delimited;
    return *this;
}

const std::vector<std::string>& SensorManifest::getLabels() const
{
    return m_labels;
}

SensorManifest& SensorManifest::setLabels(std::initializer_list<std::string> labels)
{
    m_labels = labels;
    return *this;
}
}    // namespace wolkabout
