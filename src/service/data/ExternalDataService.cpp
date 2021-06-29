/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#include "ExternalDataService.h"

#include "core/model/Message.h"
#include "core/protocol/DataProtocol.h"
#include "core/utilities/Logger.h"

#include <algorithm>

namespace wolkabout
{
void ExternalDataService::addSensorReading(const std::string& deviceKey, const SensorReading& reading)
{
    const std::shared_ptr<Message> message =
      m_protocol.makeMessage(deviceKey, {std::make_shared<SensorReading>(reading)});

    addMessage(message);
}

void ExternalDataService::addSensorReadings(const std::string& deviceKey, const std::vector<SensorReading>& readings)
{
    if (readings.empty())
    {
        return;
    }

    std::vector<std::shared_ptr<SensorReading>> parsableReadings;

    std::transform(readings.begin(), readings.end(), std::back_inserter(parsableReadings),
                   [](const SensorReading& reading) { return std::make_shared<SensorReading>(reading); });

    const std::shared_ptr<Message> message = m_protocol.makeMessage(deviceKey, parsableReadings);

    addMessage(message);
}

void ExternalDataService::addAlarm(const std::string& deviceKey, const Alarm& alarm)
{
    const std::shared_ptr<Message> message = m_protocol.makeMessage(deviceKey, {std::make_shared<Alarm>(alarm)});

    addMessage(message);
}

void ExternalDataService::addActuatorStatus(const std::string& deviceKey, const ActuatorStatus& status)
{
    const std::shared_ptr<Message> message =
      m_protocol.makeMessage(deviceKey, {std::make_shared<ActuatorStatus>(status)});

    addMessage(message);
}

void ExternalDataService::addConfiguration(const std::string& deviceKey,
                                           const std::vector<ConfigurationItem>& configurations)
{
    const std::shared_ptr<Message> message = m_protocol.makeMessage(deviceKey, configurations);

    addMessage(message);
}

void ExternalDataService::requestActuatorStatusesForDevice(const std::string& /*deviceKey*/)
{
    LOG(WARN) << "Not requesting actuator status for device";
}

void ExternalDataService::requestActuatorStatusesForAllDevices()
{
    LOG(WARN) << "Not handling message for devices";
}

void ExternalDataService::handleMessageForDevice(std::shared_ptr<Message> /*message*/)
{
    LOG(WARN) << "Not handling message for device";
}

}    // namespace wolkabout
