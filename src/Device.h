/*
 * Copyright 2017 WolkAbout Technology s.r.o.
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

#ifndef DEVICE_H
#define DEVICE_H

#include <string>
#include <vector>

namespace wolkabout
{
class Device
{
public:
    Device(std::string deviceKey = "", std::string devicePassword = "",
           std::vector<std::string> actuatorReferences = {});

    /**
     * @brief Sets device key
     * @param key Device key provided by WolkAbout IoT Cloud
     * @return Reference to current wolkabout::Device instance (Provides fluent interface)
     */
    Device& setDeviceKey(const std::string& key);
    /**
     * @brief Gets device key
     * @return Reference to current wolkabout::Device instance (Provides fluent interface)
     */
    const std::string& getDeviceKey();

    /**
     * @brief Sets device password
     * @param password Device password provided by WolkAbout IoT Cloud
     * @return Reference to current wolkabout::Device instance (Provides fluent interface)
     */
    Device& setPasswordPassword(const std::string& password);
    /**
     * @brief Gets device password
     * @return
     */
    const std::string& getPassword();

    /**
     * @brief Set actuator references for device
     * @param actuators Actuator references
     * @return Reference to current wolkabout::Device instance (Provides fluent interface)
     */
    Device& setActuatorReferences(const std::vector<std::string>& actuators);
    /**
     * @brief Gets actuator references for device
     * @return Actuator references
     */
    const std::vector<std::string> getActuatorReferences();

    virtual ~Device() = default;

private:
    std::string m_deviceKey;
    std::string m_devicePassword;
    std::vector<std::string> m_actuatorReferences;
};
}

#endif
