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

#ifndef SENSORMANIFEST_H
#define SENSORMANIFEST_H

#include <initializer_list>
#include <string>
#include <vector>

namespace wolkabout
{
class SensorManifest
{
public:
    enum class DataType
    {
        STRING,
        NUMERIC,
        BOOLEAN
    };

    SensorManifest() = default;

    SensorManifest(std::string name, std::string reference, std::string description, std::string unit,
                   std::string readingType, SensorManifest::DataType dataType, unsigned int precision, double minimum,
                   double maximum);

    SensorManifest(std::string name, std::string reference, std::string description, std::string unit,
                   std::string readingType, SensorManifest::DataType dataType, unsigned int precision, double minimum,
                   double maximum, std::string delimiter, std::vector<std::string> labels);

    virtual ~SensorManifest() = default;

    const std::string& getName() const;
    SensorManifest& setName(const std::string& name);

    const std::string& getReference() const;
    SensorManifest& setReference(const std::string& reference);

    const std::string& getDescription() const;
    SensorManifest& setDescription(const std::string description);

    const std::string& getUnit() const;
    SensorManifest& setUnit(const std::string& unit);

    const std::string& getReadingType() const;
    SensorManifest& setReadingType(const std::string& readingType);

    SensorManifest::DataType getDataType() const;
    SensorManifest& setDataType(SensorManifest::DataType dataType);

    unsigned int getPrecision() const;
    SensorManifest& setPrecision(unsigned int precision);

    double getMinimum() const;
    SensorManifest& setMinimum(double minimum);

    double getMaximum() const;
    SensorManifest& setMaximum(double maximum);

    const std::string& getDelimiter() const;
    SensorManifest& setDelimiter(const std::string& delimited);

    const std::vector<std::string>& getLabels() const;
    SensorManifest& setLabels(std::initializer_list<std::string> labels);
    SensorManifest& setLabels(const std::vector<std::string>& labels);

    bool operator==(SensorManifest& rhs) const;
    bool operator!=(SensorManifest& rhs) const;

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

#endif    // SENSORMANIFEST_H
