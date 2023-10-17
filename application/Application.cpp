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
#include "core/utility/Logger.h"
#include "core/utility/StringUtils.h"
#include "gateway/WolkGateway.h"
#include "wolk/service/firmware_update/debian/DebianPackageInstaller.h"

#include <chrono>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <iostream>

using namespace wolkabout;
using namespace wolkabout::connect;
using namespace wolkabout::gateway;
using namespace wolkabout::legacy;

namespace
{
class DefaultDataProvider : public DataProvider
{
public:
    void setDataHandler(DataHandler* handler, const std::string& gatewayKey) override
    {
        LOG(DEBUG) << "Received DataHandler for gateway '" << gatewayKey << "'.";

        m_handler = handler;
        m_gatewayKey = gatewayKey;
    }

    void onConnected()
    {
        if (m_handler != nullptr)
        {
            m_handler->pullFeedValues("AD1");
            m_handler->addReading("AD1", Reading{"T", std::uint64_t{25}});
        }
    }

    void onReadingData(const std::string& deviceKey, std::map<std::uint64_t, std::vector<Reading>> readings) override
    {
        LOG(DEBUG) << "Received reading data for device '" << deviceKey << "'.";
        for (const auto& timestamp : readings)
            for (const auto& reading : timestamp.second)
                LOG(DEBUG) << "\tReference: " << reading.getReference() << " | Value: " << reading.getStringValue()
                           << " | Timestamp: " << std::to_string(timestamp.first);
    }

    void onParameterData(const std::string& deviceKey, std::vector<Parameter> parameters) override
    {
        LOG(DEBUG) << "Received parameter data for device '" << deviceKey << "'.";
        for (const auto& parameter : parameters)
            LOG(DEBUG) << "\tParameter: " << toString(parameter.first) << " | Value: " << parameter.second;
    }

private:
    std::string m_gatewayKey;
    DataHandler* m_handler = nullptr;
};

wolkabout::legacy::LogLevel parseLogLevel(const std::string& levelStr)
{
    const std::string str = wolkabout::legacy::StringUtils::toUpperCase(levelStr);
    const auto logLevel = [&]() -> wolkabout::legacy::LogLevel {
        if (str == "TRACE")
            return wolkabout::legacy::LogLevel::TRACE;
        else if (str == "DEBUG")
            return wolkabout::legacy::LogLevel::DEBUG;
        else if (str == "INFO")
            return wolkabout::legacy::LogLevel::INFO;
        else if (str == "WARN")
            return wolkabout::legacy::LogLevel::WARN;
        else if (str == "ERROR")
            return wolkabout::legacy::LogLevel::ERROR;

        throw std::logic_error("Unable to parse log level.");
    }();

    return logLevel;
}
}    // namespace

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "WolkGateway Application: Usage -  " << argv[0] << " [gatewayConfigurationFilePath] [logLevel]";
        return -1;
    }

    const auto level = [&] {
        if (argc > 2)
        {
            const std::string logLevelStr{argv[2]};
            try
            {
                return parseLogLevel(logLevelStr);
            }
            catch (std::logic_error& e)
            {
                return wolkabout::legacy::LogLevel::INFO;
            }
        }
        return wolkabout::legacy::LogLevel::INFO;
    }();
    wolkabout::legacy::Logger::init(level, wolkabout::legacy::Logger::Type::CONSOLE);

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

    auto gateway = wolkabout::Device(gatewayConfiguration.getKey(), gatewayConfiguration.getPassword(),
                                     wolkabout::OutboundDataMode::PUSH);
    auto dataProvider = std::unique_ptr<DefaultDataProvider>{new DefaultDataProvider};

    auto installer = [&] {
        auto aptInstaller = std::unique_ptr<APTPackageInstaller>{new APTPackageInstaller};
        auto systemdManager = std::unique_ptr<SystemdServiceInterface>{new SystemdServiceInterface};
        return std::unique_ptr<DebianPackageInstaller>{
          new DebianPackageInstaller{"wolkgateway", std::move(aptInstaller), std::move(systemdManager)}};
    }();
    installer->start();

    auto builder = std::move(WolkGateway::newBuilder(gateway)
                               .withFileTransfer("./files")
                               .withFirmwareUpdate(std::move(installer))
                               .setMqttKeepAlive(gatewayConfiguration.getKeepAliveSec())
                               .platformHost(gatewayConfiguration.getPlatformMqttUri())
                               .withInternalDataService(gatewayConfiguration.getLocalMqttUri())
                               .withPlatformRegistration()
                               .withPlatformStatusService()
                               .withLocalRegistration());
    if (!gatewayConfiguration.getPlatformTrustStore().empty())
    {
        builder.platformTrustStore(gatewayConfiguration.getPlatformTrustStore());
    }

    auto wolk = builder.build();
    wolk->setConnectionStatusListener([&](bool connected) {
        if (connected)
            dataProvider->onConnected();
    });

    wolk->connect();

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
