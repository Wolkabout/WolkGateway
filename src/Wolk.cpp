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
#include "ActuatorStatusProvider.h"
#include "WolkBuilder.h"
#include "model/ActuatorCommand.h"
#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/Device.h"
#include "model/SensorReading.h"
#include "service/publish/PublishService.h"

#include <memory>
#include <sstream>
#include <string>
#include <utility>

#define INSTANTIATE_ADD_SENSOR_READING_FOR(x) \
    template void Wolk::addSensorReading<x>(const std::string& reference, x value, unsigned long long rtc)

namespace wolkabout
{
WolkBuilder Wolk::newBuilder(Device device)
{
    return WolkBuilder(device);
}

template <> void Wolk::addSensorReading(const std::string& reference, std::string value, unsigned long long rtc)
{
    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    std::unique_ptr<SensorReading> sensorReading(new SensorReading(std::move(value), reference, rtc));
    m_publishService->addReading(std::move(sensorReading));
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

void Wolk::addAlarm(const std::string& reference, const std::string& value, unsigned long long rtc)
{
    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    std::unique_ptr<Alarm> event(new Alarm(value, reference, rtc));
    m_publishService->addReading(std::move(event));
}

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

void Wolk::connect()
{
    m_publishService->start();

    for (const std::string& actuatorReference : m_device.getActuatorReferences())
    {
        publishActuatorStatus(actuatorReference);
    }
}

void Wolk::disconnect()
{
    if (m_publishService)
    {
        m_publishService->stop();
    }
}

Wolk::Wolk(std::shared_ptr<PublishService> publishService, Device device)
: m_device(std::move(device))
, m_actuationHandlerLambda(nullptr)
, m_actuatorStatusProviderLambda(nullptr)
, m_publishService(publishService)
{
}

void Wolk::addActuatorStatus(const std::string& reference, const ActuatorStatus& actuatorStatus)
{
    std::unique_ptr<ActuatorStatus> status(
      new ActuatorStatus(actuatorStatus.getValue(), reference, actuatorStatus.getState()));

    m_publishService->addReading(std::move(status));
}

void Wolk::handleActuatorCommand(const ActuatorCommand& actuatorCommand)
{
    if (actuatorCommand.getType() == ActuatorCommand::Type::STATUS)
    {
        publishActuatorStatus(actuatorCommand.getReference());
    }
    else if (actuatorCommand.getType() == ActuatorCommand::Type::SET)
    {
        handleSetActuator(actuatorCommand);
        publishActuatorStatus(actuatorCommand.getReference());
    }
}

void Wolk::handleSetActuator(const ActuatorCommand& actuatorCommand)
{
    if (auto provider = m_actuationHandler.lock())
    {
        provider->handleActuation(actuatorCommand.getReference(), actuatorCommand.getValue());
    }
    else if (m_actuationHandlerLambda)
    {
        m_actuationHandlerLambda(actuatorCommand.getReference(), actuatorCommand.getValue());
    }
}

unsigned long long Wolk::currentRtc()
{
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
}
}
