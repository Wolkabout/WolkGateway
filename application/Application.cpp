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
#include "core/utilities/Logger.h"
#include "core/utilities/StringUtils.h"
#include "gateway/WolkGateway.h"

#include <chrono>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>

using namespace wolkabout;
using namespace wolkabout::gateway;

namespace
{
/**
 * This is a function that will generate a random Temperature value for us.
 *
 * @return A new Temperature value, in the range of -20 to 80.
 */
std::int16_t generateRandomValue()
{
    // Here we will create the random engine and distribution
    static auto engine =
      std::mt19937(static_cast<std::uint64_t>(std::chrono::system_clock::now().time_since_epoch().count()));
    static auto distribution = std::uniform_real_distribution<>(-20, 80);

    // And generate a random value
    return static_cast<std::int16_t>(distribution(engine));
}

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

    void receiveReadingData(const std::string& deviceKey,
                            std::map<std::uint64_t, std::vector<Reading>> readings) override
    {
        LOG(DEBUG) << "Received reading data for device '" << deviceKey << "'.";
        for (const auto& timestamp : readings)
            for (const auto& reading : timestamp.second)
                LOG(DEBUG) << "\tReference: " << reading.getReference() << " | Value: " << reading.getStringValue()
                           << " | Timestamp: " << std::to_string(timestamp.first);
    }

    void receiveParameterData(const std::string& deviceKey, std::vector<Parameter> parameters) override
    {
        LOG(DEBUG) << "Received parameter data for device '" << deviceKey << "'.";
        for (const auto& parameter : parameters)
            LOG(DEBUG) << "\tParameter: " << toString(parameter.first) << " | Value: " << parameter.second;
    }

private:
    std::string m_gatewayKey;
    DataHandler* m_handler = nullptr;
};

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
    wolkabout::Logger::init(wolkabout::LogLevel::INFO, wolkabout::Logger::Type::CONSOLE);

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

    auto gateway = wolkabout::Device(gatewayConfiguration.getKey(), gatewayConfiguration.getPassword(),
                                     wolkabout::OutboundDataMode::PUSH);
    auto dataProvider = std::unique_ptr<DefaultDataProvider>{new DefaultDataProvider};

    auto builder = std::move(WolkGateway::newBuilder(gateway)
                               .withFileTransfer("./files")
                               .setMqttKeepAlive(gatewayConfiguration.getKeepAliveSec())
                               .platformHost(gatewayConfiguration.getPlatformMqttUri())
                               .withInternalDataService(gatewayConfiguration.getLocalMqttUri())
                               .withPlatformRegistration()
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

    std::this_thread::sleep_for(std::chrono::seconds(5));
    wolk->addReading("TF", generateRandomValue());
    wolk->publish();

    while (true)
        std::this_thread::sleep_for(std::chrono::seconds(1));

    return 0;
}
