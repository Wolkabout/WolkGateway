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

#include "model/WolkOptional.h"
#include <string>

namespace wolkabout
{
class GatewayConfiguration
{
public:
    GatewayConfiguration() = default;

    GatewayConfiguration(std::string key, std::string password, std::string platformMqttUri, std::string localMqttUri);

    const std::string& getKey() const;
    const std::string& getPassword() const;

    const std::string& getPlatformMqttUri() const;
    const std::string& getLocalMqttUri() const;

    void setKeepAliveEnabled(bool value);

    const WolkOptional<bool>& getKeepAliveEnabled() const;

    static wolkabout::GatewayConfiguration fromJson(const std::string& gatewayConfigurationFile);

private:
    std::string m_key;
    std::string m_password;

    std::string m_platformMqttUri;
    std::string m_localMqttUri;

    WolkOptional<bool> m_keepAliveEnabled;

    static const std::string KEY;
    static const std::string PASSWORD;
    static const std::string PLATFORM_URI;
    static const std::string LOCAL_URI;
    static const std::string KEEP_ALIVE;
};
}    // namespace wolkabout
