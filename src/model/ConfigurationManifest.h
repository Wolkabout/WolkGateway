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

#ifndef CONFIGMANIFEST_H
#define CONFIGMANIFEST_H

#include <string>

namespace wolkabout
{
class ConfigurationManifest
{
public:
    enum class DataType
    {
        STRING,
        NUMERIC,
        BOOLEAN
    };

    ConfigurationManifest() = default;
    ConfigurationManifest(std::string name, std::string reference, std::string description, std::string unit,
                          ConfigurationManifest::DataType dataType, double minimum, double maximum,
                          std::string collapseKey, std::string defaultValue, std::string nullValue, bool isOptional,
                          unsigned int size, std::string delimiter);

    virtual ~ConfigurationManifest() = default;

    const std::string& getName() const;
    ConfigurationManifest& setName(const std::string& name);

    const std::string& getReference() const;
    ConfigurationManifest& setReference(const std::string& reference);

    const std::string& getDescription() const;
    ConfigurationManifest& setDescription(const std::string& description);

    const std::string& getUnit() const;
    ConfigurationManifest& setUnit(const std::string& unit);

    ConfigurationManifest::DataType getDataType() const;
    ConfigurationManifest& setDataType(ConfigurationManifest::DataType dataType);

    double getMinimum() const;
    ConfigurationManifest& setMinimum(double minimum);

    double getMaximum() const;
    ConfigurationManifest& setMaximum(double maximum);

    unsigned int getSize() const;
    ConfigurationManifest& setSize(unsigned int size);

    const std::string& getDelimiter() const;
    ConfigurationManifest& setDelimiter(const std::string delimiter);

    const std::string& getCollapseKey() const;
    ConfigurationManifest& setCollapseKey(const std::string& collapseKey);

    const std::string& getDefaultValue() const;
    ConfigurationManifest& setDefaultValue(const std::string& defaultValue);

    const std::string& getNullValue() const;
    ConfigurationManifest& setNullValue(const std::string& nullValue);

    bool isOptional() const;
    ConfigurationManifest& setIsOptional(bool isOptional);

    bool operator==(ConfigurationManifest& rhs) const;
    bool operator!=(ConfigurationManifest& rhs) const;

private:
    std::string m_name;
    std::string m_reference;
    std::string m_description;
    std::string m_unit;
    ConfigurationManifest::DataType m_dataType;

    double m_minimum;
    double m_maximum;

    unsigned int m_size;
    std::string m_delimiter;
    std::string m_collapseKey;

    std::string m_defaultValue;
    std::string m_nullValue;

    bool m_isOptional;
};
}    // namespace wolkabout

#endif    // CONFIGMANIFEST_H
