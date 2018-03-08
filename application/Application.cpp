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
#include "connectivity/json/JsonProtocol.h"
#include "utilities/ConsoleLogger.h"
#include "utilities/StringUtils.h"

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>

namespace
{
void setupLogger()
{
    auto logger = std::unique_ptr<wolkabout::ConsoleLogger>(new wolkabout::ConsoleLogger());
    logger->setLogLevel(wolkabout::LogLevel::INFO);
    wolkabout::Logger::setInstance(std::move(logger));
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
            wolkabout::Logger::getInstance()->setLogLevel(level);
        }
        catch (std::logic_error& e)
        {
            LOG(ERROR) << "WolkGateway Application: " << e.what();
        }
    }

    if (gatewayConfiguration.getProtocol() != wolkabout::JsonProtocol::getName())
    {
        LOG(ERROR) << "WolkGateway Application: Unsupported protocol '" << gatewayConfiguration.getProtocol()
                   << "' specified in gateway configuration file";
        return -1;
    }

    wolkabout::Device device(gatewayConfiguration.getName(), gatewayConfiguration.getKey(),
                             gatewayConfiguration.getPassword());
    std::unique_ptr<wolkabout::Wolk> wolk = wolkabout::Wolk::newBuilder(device)
                                              .withDataProtocol<wolkabout::JsonProtocol>()
                                              .gatewayHost(gatewayConfiguration.getLocalMqttUri())
                                              .platformHost(gatewayConfiguration.getPlatformMqttUri())
                                              .build();

    wolk->connect();
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
