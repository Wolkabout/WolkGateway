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

#ifndef WOLK_H
#define WOLK_H

#include "ActuationHandler.h"
#include "ActuatorCommand.h"
#include "ActuatorStatusProvider.h"
#include "Device.h"
#include "MqttService.h"
#include "PublishingService.h"
#include "ReadingsBuffer.h"

#include <exception>
#include <functional>
#include <memory>
#include <string>

namespace wolkabout
{
class Wolk;
class WolkBuilder final
{
    friend class Wolk;

public:
    ~WolkBuilder() = default;

    /**
     * @brief Allows passing of URI to custom WolkAbout IoT Cloud instance
     * @param host Server URI
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& toHost(const std::string& host);

    /**
     * @brief Sets actuation handler
     * @param actuationHandler Lambda that handles actuation requests
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuationHandler(
      std::function<void(const std::string& reference, const std::string& value)> actuationHandler);
    /**
     * @brief Sets actuation handler
     * @param actuationHandler Instance of wolkabout::ActuationHandler
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuationHandler(std::weak_ptr<ActuationHandler> actuationHandler);

    /**
     * @brief Sets actuation status provider
     * @param actuatorStatusProvider Lambda that provides ActuatorStatus by reference of requested actuator
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuatorStatusProvider(
      std::function<ActuatorStatus(const std::string& reference)> actuatorStatusProvider);
    /**
     * @brief Sets actuation status provider
     * @param actuatorStatusProvider Instance of wolkabout::ActuatorStatusProvider
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuatorStatusProvider(std::weak_ptr<ActuatorStatusProvider> actuatorStatusProvider);

    /**
     * @brief Connect device to WolkAbout IoT Platform
     * @return wolkabout::Wolk instance wrapped in std::unique_ptr
     */
    std::unique_ptr<Wolk> connect();

private:
    WolkBuilder(Device device);
    WolkBuilder(WolkBuilder&&) = default;

    std::unique_ptr<Wolk> m_wolk;
};

class Wolk : public MqttServiceListener
{
    friend class WolkBuilder;

public:
    virtual ~Wolk() = default;

    /**
     * @brief Initiates wolkabout::WolkBuilder that connects device to WolkAbout IoT Cloud
     * @param device wolkabout::Device to connect to WolkAbout IoT Platform
     * @return wolkabout::WolkBuilder instance
     */
    static WolkBuilder connectDevice(Device device);

    /**
     * @brief Publishes sensor reading to WolkAbout IoT Cloud
     * @param reference Sensor reference
     * @param value Sensor value<br>
     *              Supported types:<br>
     *               - bool<br>
     *               - float<br>
     *               - double<br>
     *               - signed int<br>
     *               - signed long int<br>
     *               - signed long long int<br>
     *               - unsigned int<br>
     *               - unsigned long int<br>
     *               - unsigned long long int<br>
     *               - string<br>
     *               - char*<br>
     *               - const char*<br>
     * @param rtc Reading POSIX time - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    template <typename T> void addSensorReading(const std::string& reference, T value, unsigned long long int rtc = 0);

    /**
     * @brief Publishes alarm to WolkAbout IoT Cloud
     * @param reference Alarm reference
     * @param value Alarm value
     * @param rtc POSIX time at which event occurred - Number of seconds since 01/01/1970<br>
     *            If omitted current POSIX time is adopted
     */
    void addAlarm(const std::string& reference, const std::string& value, unsigned long long int rtc = 0);

    /**
     * @brief Invokes ActuatorStatusProvider callback to obtain actuator status
     * @param Actuator reference
     */
    void publishActuatorStatus(const std::string& reference);

private:
    Wolk(Device device);

    void connect();
    void disconnect();

    void addActuatorStatus(const std::string& reference, const ActuatorStatus& actuatorStatus);

    void handleActuatorCommand(const ActuatorCommand& actuatorCommand, const std::string& reference);

    void handleSetActuator(const ActuatorCommand& actuatorCommand, const std::string& reference);

    static unsigned long long int currentRtc();

    virtual void messageArrived(std::string topic, std::string message) override;

    Device m_device;
    std::string m_host;

    std::function<void(std::string, std::string)> m_actuationHandlerLambda;
    std::weak_ptr<ActuationHandler> m_actuationHandler;

    std::function<ActuatorStatus(std::string)> m_actuatorStatusProviderLambda;
    std::weak_ptr<ActuatorStatusProvider> m_actuatorStatusProvider;

    std::shared_ptr<ReadingBuffer> m_readingsBuffer;

    std::shared_ptr<MqttService> m_mqttService;

    std::unique_ptr<PublishingService> m_publishingService;

    std::unique_ptr<MqttServiceListener> m_mqttServiceListener;

    static const constexpr char* WOLK_DEMO_HOST = "ssl://api-demo.wolkabout.com:8883";
};
}

#endif
