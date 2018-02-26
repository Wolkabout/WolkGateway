#include "ConfigurationManifest.h"

#include <string>
#include <utility>

namespace wolkabout
{
ConfigurationManifest::ConfigurationManifest(std::string name, std::string reference, std::string description,
                                             std::string unit, ConfigurationManifest::DataType dataType, double minimum,
                                             double maximum, std::string collapseKey, std::string defaultValue,
                                             std::string nullValue, bool isOptional, unsigned int size,
                                             std::string delimiter)
: m_name(std::move(name))
, m_reference(std::move(reference))
, m_description(std::move(description))
, m_unit(std::move(unit))
, m_dataType(dataType)
, m_minimum(minimum)
, m_maximum(maximum)
, m_size(size)
, m_delimiter(std::move(delimiter))
, m_collapseKey(std::move(collapseKey))
, m_defaultValue(std::move(defaultValue))
, m_nullValue(std::move(nullValue))
, m_isOptional(isOptional)
{
}

const std::string& ConfigurationManifest::getName() const
{
    return m_name;
}

ConfigurationManifest& ConfigurationManifest::setName(const std::string& name)
{
    m_name = name;
    return *this;
}

const std::string& ConfigurationManifest::getReference() const
{
    return m_reference;
}

ConfigurationManifest& ConfigurationManifest::setReference(const std::string& reference)
{
    m_reference = reference;
    return *this;
}

const std::string& ConfigurationManifest::getDescription() const
{
    return m_description;
}

ConfigurationManifest& ConfigurationManifest::setDescription(const std::string& description)
{
    m_description = description;
    return *this;
}

const std::string& ConfigurationManifest::getUnit() const
{
    return m_unit;
}

ConfigurationManifest& ConfigurationManifest::setUnit(const std::string& unit)
{
    m_unit = unit;
    return *this;
}

ConfigurationManifest::DataType ConfigurationManifest::getDataType() const
{
    return m_dataType;
}

ConfigurationManifest& ConfigurationManifest::setDataType(ConfigurationManifest::DataType dataType)
{
    m_dataType = dataType;
    return *this;
}

double ConfigurationManifest::getMinimum() const
{
    return m_minimum;
}

ConfigurationManifest& ConfigurationManifest::setMinimum(double minimum)
{
    m_minimum = minimum;
    return *this;
}

double ConfigurationManifest::getMaximum() const
{
    return m_maximum;
}

ConfigurationManifest& ConfigurationManifest::setMaximum(double maximum)
{
    m_maximum = maximum;
    return *this;
}

unsigned int ConfigurationManifest::getSize() const
{
    return m_size;
}

ConfigurationManifest& ConfigurationManifest::setSize(unsigned int size)
{
    m_size = size;
    return *this;
}

const std::string& ConfigurationManifest::getDelimiter() const
{
    return m_delimiter;
}

ConfigurationManifest& ConfigurationManifest::setDelimiter(const std::string delimiter)
{
    m_delimiter = delimiter;
    return *this;
}

const std::string& ConfigurationManifest::getCollapseKey() const
{
    return m_collapseKey;
}

ConfigurationManifest& ConfigurationManifest::setCollapseKey(const std::string& collapseKey)
{
    m_collapseKey = collapseKey;
    return *this;
}

const std::string& ConfigurationManifest::getDefaultValue() const
{
    return m_defaultValue;
}

ConfigurationManifest& ConfigurationManifest::setDefaultValue(const std::string& defaultValue)
{
    m_defaultValue = defaultValue;
    return *this;
}

const std::string& ConfigurationManifest::getNullValue() const
{
    return m_nullValue;
}

ConfigurationManifest& ConfigurationManifest::setNullValue(const std::string& nullValue)
{
    m_nullValue = nullValue;
    return *this;
}

bool ConfigurationManifest::isOptional() const
{
    return m_isOptional;
}

ConfigurationManifest& ConfigurationManifest::setIsOptional(bool isOptional)
{
    m_isOptional = isOptional;
    return *this;
}

}    // namespace wolkabout
