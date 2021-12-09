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

#include "Configuration.h"
#include "Wolk.h"
#include "core/utilities/Logger.h"
#include "core/utilities/StringUtils.h"
#include "protocol/json/JsonGatewayDataProtocol.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
void setupLogger()
{
    wolkabout::Logger::init(wolkabout::LogLevel::INFO, wolkabout::Logger::Type::CONSOLE);
}

wolkabout::LogLevel parseLogLevel(const std::string& levelStr)
{
    const std::string str = wolkabout::StringUtils::toUpperCase(levelStr);
    const auto logLevel = [&]() -> wolkabout::LogLevel {
        if (str == "TRACE")
            return wolkabout::LogLevel::TRACE;
        else if (str == "DEBUG")
            return wolkabout::LogLevel::DEBUG;
        else if (str == "INFO")
            return wolkabout::LogLevel::INFO;
        else if (str == "WARN")
            return wolkabout::LogLevel::WARN;
        else if (str == "ERROR")
            return wolkabout::LogLevel::ERROR;

        throw std::logic_error("Unable to parse log level.");
    }();

    return logLevel;
}
}    // namespace

int main(int argc, char** argv)
{
    setupLogger();

    if (argc < 2)
    {
        LOG(ERROR) << "WolkGateway Application: Usage -  " << argv[0] << " [gatewayConfigurationFilePath] [logLevel]";
        return -1;
    }

    wolkabout::GatewayConfiguration gatewayConfiguration;
    try
    {
        gatewayConfiguration = wolkabout::GatewayConfiguration::fromJson(argv[1]);
    }
    catch (std::logic_error& e)
    {
        LOG(ERROR) << "WolkGateway Application: Unable to parse gateway configuration file. Reason: " << e.what();
        return -1;
    }

    if (argc > 2)
    {
        const std::string logLevelStr{argv[2]};
        try
        {
            wolkabout::LogLevel level = parseLogLevel(logLevelStr);
            wolkabout::Logger::getInstance().setLevel(level);
        }
        catch (std::logic_error& e)
        {
            LOG(ERROR) << "WolkGateway Application: " << e.what();
        }
    }

    wolkabout::GatewayDevice device(gatewayConfiguration.getKey(), gatewayConfiguration.getPassword(),
                                    gatewayConfiguration.getSubdeviceManagement());
    auto builder = std::move(wolkabout::Wolk::newBuilder(device)
                               .setMqttKeepAlive(gatewayConfiguration.getKeepAliveSec())
                               .gatewayHost(gatewayConfiguration.getLocalMqttUri())
                               .platformHost(gatewayConfiguration.getPlatformMqttUri()));

    if (gatewayConfiguration.getPlatformTrustStore())
    {
        builder.platformTrustStore(gatewayConfiguration.getPlatformTrustStore().value());
    }

    std::unique_ptr<wolkabout::Wolk> wolk = builder.build();

    wolk->connect();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
