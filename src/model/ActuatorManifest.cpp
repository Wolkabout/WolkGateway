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

#include "ActuatorManifest.h"

#include <initializer_list>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace wolkabout
{
ActuatorManifest::ActuatorManifest(std::string name, std::string reference, std::string description, std::string unit,
                                   std::string readingType, ActuatorManifest::DataType dataType, unsigned int precision,
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

ActuatorManifest::ActuatorManifest(std::string name, std::string reference, std::string description, std::string unit,
                                   std::string readingType, ActuatorManifest::DataType dataType, unsigned int precision,
                                   double minimum, double maximum, std::string delimiter,
                                   std::vector<std::string> labels)
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

const std::string& ActuatorManifest::getName() const
{
    return m_name;
}

wolkabout::ActuatorManifest& ActuatorManifest::setName(const std::string& name)
{
    m_name = name;
    return *this;
}

const std::string& ActuatorManifest::getReference() const
{
    return m_reference;
}

ActuatorManifest& ActuatorManifest::setReference(const std::string& reference)
{
    m_reference = reference;
    return *this;
}

const std::string& ActuatorManifest::getDescription() const
{
    return m_description;
}

ActuatorManifest& ActuatorManifest::setDescription(const std::string description)
{
    m_description = description;
    return *this;
}

const std::string& ActuatorManifest::getUnit() const
{
    return m_unit;
}

ActuatorManifest& ActuatorManifest::setUnit(const std::string& unit)
{
    m_unit = unit;
    return *this;
}

const std::string& ActuatorManifest::getReadingType() const
{
    return m_readingType;
}

ActuatorManifest& ActuatorManifest::setReadingType(const std::string& readingType)
{
    m_readingType = readingType;
    return *this;
}

ActuatorManifest::DataType ActuatorManifest::getDataType() const
{
    return m_dataType;
}

ActuatorManifest& ActuatorManifest::setDataType(ActuatorManifest::DataType dataType)
{
    m_dataType = dataType;
    return *this;
}

unsigned int ActuatorManifest::getPrecision() const
{
    return m_precision;
}

ActuatorManifest& ActuatorManifest::setPrecision(unsigned int precision)
{
    m_precision = precision;
    return *this;
}

double ActuatorManifest::getMinimum() const
{
    return m_minimum;
}

ActuatorManifest& ActuatorManifest::setMinimum(double minimum)
{
    m_minimum = minimum;
    return *this;
}

double ActuatorManifest::getMaximum() const
{
    return m_maximum;
}

ActuatorManifest& ActuatorManifest::setMaximum(double maximum)
{
    m_maximum = maximum;
    return *this;
}

const std::string& ActuatorManifest::getDelimiter() const
{
    return m_delimiter;
}

ActuatorManifest& ActuatorManifest::setDelimiter(const std::string& delimited)
{
    m_delimiter = delimited;
    return *this;
}

const std::vector<std::string>& ActuatorManifest::getLabels() const
{
    return m_labels;
}

ActuatorManifest& ActuatorManifest::setLabels(std::initializer_list<std::string> labels)
{
    m_labels = labels;
    return *this;
}

ActuatorManifest& ActuatorManifest::setLabels(const std::vector<std::string>& labels)
{
    m_labels = labels;
    return *this;
}

bool ActuatorManifest::operator==(ActuatorManifest& rhs) const
{
    if (m_name != rhs.m_name || m_reference != rhs.m_reference || m_description != rhs.m_description ||
        m_unit != rhs.m_unit || m_readingType != rhs.m_readingType || m_dataType != rhs.m_dataType ||
        m_precision != rhs.m_precision)
    {
        return false;
    }

    if (m_minimum != rhs.m_minimum || m_maximum != rhs.m_maximum)
    {
        return false;
    }

    if (m_delimiter != rhs.m_delimiter)
    {
        return false;
    }

    if (m_labels.size() != rhs.m_labels.size())
    {
        return false;
    }

    for (unsigned long long int i = 0; i < m_labels.size(); ++i)
    {
        if (m_labels[i] != rhs.m_labels[i])
        {
            return false;
        }
    }

    return true;
}

bool ActuatorManifest::operator!=(ActuatorManifest& rhs) const
{
    return !(*this == rhs);
}
}    // namespace wolkabout
