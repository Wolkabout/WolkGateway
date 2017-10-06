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

#include "Wolk.h"
#include "ActuationHandler.h"
#include "ActuatorCommand.h"
#include "ActuatorStatus.h"
#include "ActuatorStatusProvider.h"
#include "Device.h"
#include "JsonParser.h"
#include "MqttService.h"
#include "ReadingsBuffer.h"
#include "SensorReading.h"

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#define INSTANTIATE_ADD_SENSOR_READING_FOR(x) \
    template void Wolk::addSensorReading<x>(const std::string& reference, x value, unsigned long long rtc)

namespace wolkabout
{
WolkBuilder& WolkBuilder::toHost(const std::string& host)
{
    m_wolk->m_host = host;
    return *this;
}

WolkBuilder& WolkBuilder::actuationHandler(std::function<void(const std::string&, const std::string&)> actuationHandler)
{
    m_wolk->m_actuationHandlerLambda = actuationHandler;
    m_wolk->m_actuationHandler.reset();
    return *this;
}

WolkBuilder& WolkBuilder::actuationHandler(std::weak_ptr<ActuationHandler> actuationHandler)
{
    m_wolk->m_actuationHandler = actuationHandler;
    m_wolk->m_actuationHandlerLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::actuatorStatusProvider(
  std::function<ActuatorStatus(const std::string&)> actuatorStatusProvider)
{
    m_wolk->m_actuatorStatusProviderLambda = actuatorStatusProvider;
    m_wolk->m_actuatorStatusProvider.reset();
    return *this;
}

WolkBuilder& WolkBuilder::actuatorStatusProvider(std::weak_ptr<ActuatorStatusProvider> actuatorStatusProvider)
{
    m_wolk->m_actuatorStatusProvider = actuatorStatusProvider;
    m_wolk->m_actuatorStatusProviderLambda = nullptr;
    return *this;
}

std::unique_ptr<Wolk> WolkBuilder::connect()
{
    m_wolk->connect();
    return std::move(m_wolk);
}

WolkBuilder::WolkBuilder(Device device) : m_wolk(std::unique_ptr<Wolk>(new Wolk(device))) {}

WolkBuilder Wolk::connectDevice(Device device)
{
    return WolkBuilder(device);
}

void Wolk::addAlarm(const std::string& reference, const std::string& value, unsigned long long rtc)
{
    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    std::unique_ptr<Alarm> event(new Alarm(value, reference, rtc));
    m_readingsBuffer->addReading(std::move(event));
}

template <> void Wolk::addSensorReading(const std::string& reference, std::string value, unsigned long long rtc)
{
    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    std::unique_ptr<SensorReading> sensorReading(new SensorReading(value, reference, rtc));
    m_readingsBuffer->addReading(std::move(sensorReading));
}

template <typename T> void Wolk::addSensorReading(const std::string& reference, T value, unsigned long long rtc)
{
    addSensorReading(reference, std::to_string(value), rtc);
}

template <> void Wolk::addSensorReading(const std::string& reference, bool value, unsigned long long rtc)
{
    addSensorReading(reference, std::string(value ? "true" : "false"), rtc);
}

template <> void Wolk::addSensorReading(const std::string& reference, char* value, unsigned long long rtc)
{
    addSensorReading(reference, std::string(value), rtc);
}

template <> void Wolk::addSensorReading(const std::string& reference, const char* value, unsigned long long rtc)
{
    addSensorReading(reference, std::string(value), rtc);
}

INSTANTIATE_ADD_SENSOR_READING_FOR(float);
INSTANTIATE_ADD_SENSOR_READING_FOR(double);
INSTANTIATE_ADD_SENSOR_READING_FOR(signed int);
INSTANTIATE_ADD_SENSOR_READING_FOR(signed long int);
INSTANTIATE_ADD_SENSOR_READING_FOR(signed long long int);
INSTANTIATE_ADD_SENSOR_READING_FOR(unsigned int);
INSTANTIATE_ADD_SENSOR_READING_FOR(unsigned long int);
INSTANTIATE_ADD_SENSOR_READING_FOR(unsigned long long int);

void Wolk::publishActuatorStatus(const std::string& reference)
{
    if (auto provider = m_actuatorStatusProvider.lock())
    {
        ActuatorStatus actuatorStatus = provider->getActuatorStatus(reference);
        addActuatorStatus(reference, actuatorStatus);
    }
    else if (m_actuatorStatusProviderLambda)
    {
        ActuatorStatus actuatorStatus = m_actuatorStatusProviderLambda(reference);
        addActuatorStatus(reference, actuatorStatus);
    }
}

void Wolk::disconnect()
{
    if (m_publishingService)
    {
        m_publishingService->stop();
    }

    if (m_mqttService)
    {
        m_mqttService->disconnect();
    }
}

Wolk::Wolk(Device device)
: m_device(std::move(device))
, m_host(WOLK_DEMO_HOST)
, m_actuationHandlerLambda(nullptr)
, m_actuatorStatusProviderLambda(nullptr)
, m_readingsBuffer(std::shared_ptr<ReadingBuffer>(new ReadingBuffer()))
, m_mqttService(nullptr)
, m_publishingService(nullptr)
{
}

void Wolk::connect()
{
    m_mqttService.reset(new MqttService(m_device, m_host));
    m_mqttService->setListener(this);

    std::vector<std::string> subscriptionList;
    for (const std::string& actuatorReference : m_device.getActuatorReferences())
    {
        std::stringstream topic("");
        topic << "actuators/commands/" << m_device.getDeviceKey() << "/" << actuatorReference;
        subscriptionList.emplace_back(topic.str());
        publishActuatorStatus(actuatorReference);
    }
    m_mqttService->setSubscriptionList(subscriptionList).connect();

    m_publishingService.reset(new PublishingService(m_mqttService, m_readingsBuffer, m_device.getDeviceKey()));
    m_publishingService->start();
}

void Wolk::addActuatorStatus(const std::string& reference, const ActuatorStatus& actuatorStatus)
{
    std::unique_ptr<ActuatorStatus> status(
      new ActuatorStatus(actuatorStatus.getValue(), reference, actuatorStatus.getState()));

    m_readingsBuffer->addReading(std::move(status));
}

void Wolk::handleActuatorCommand(const ActuatorCommand& actuatorCommand, const std::string& reference)
{
    if (actuatorCommand.getType() == ActuatorCommand::Type::STATUS)
    {
        publishActuatorStatus(reference);
    }
    else if (actuatorCommand.getType() == ActuatorCommand::Type::SET)
    {
        handleSetActuator(actuatorCommand, reference);
        publishActuatorStatus(reference);
    }
}

void Wolk::handleSetActuator(const ActuatorCommand& actuatorCommand, const std::string& reference)
{
    if (auto provider = m_actuationHandler.lock())
    {
        provider->handleActuation(reference, actuatorCommand.getValue());
    }
    else if (m_actuationHandlerLambda)
    {
        m_actuationHandlerLambda(reference, actuatorCommand.getValue());
    }
}

unsigned long long Wolk::currentRtc()
{
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
}

void Wolk::messageArrived(std::string topic, std::string message)
{
    const size_t referencePosition = topic.find_last_of('/');
    if (referencePosition == std::string::npos)
    {
        return;
    }

    const std::string reference = topic.substr(referencePosition + 1);

    ActuatorCommand actuatorCommand;
    JsonParser::fromJson(message, actuatorCommand);

    handleActuatorCommand(std::move(actuatorCommand), reference);
}
}
