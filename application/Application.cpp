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

#include <chrono>
#include <string>
#include <thread>
#include <stdexcept>

namespace
{
void setupLogger()
{
    auto logger = std::unique_ptr<wolkabout::ConsoleLogger>(new wolkabout::ConsoleLogger());
    logger->setLogLevel(wolkabout::LogLevel::INFO);
    wolkabout::Logger::setInstance(std::move(logger));
}
}

int main(int argc, char** argv)
{
    setupLogger();
    
    if (argc != 2)
    {
        LOG(ERROR) << "WolkGateway Application: Usage -  " << argv[0] << " [gatewayConfigurationFilePath]";
        return -1;
    }
    
    wolkabout::GatewayConfiguration gatewayConfiguration = wolkabout::GatewayConfiguration::fromJson(argv[1]);
    
    if (gatewayConfiguration.getProtocol() != wolkabout::JsonProtocol::getName())
    {
        throw std::logic_error("Unsupported protocol '" + gatewayConfiguration.getProtocol() + "' specified in gateway configuration file.");
    }

    wolkabout::Device device(gatewayConfiguration.getName(), gatewayConfiguration.getKey(), gatewayConfiguration.getPassword());
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
