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

#include "OutboundMessageFactory.h"
#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/OutboundMessage.h"
#include "model/SensorReading.h"
#include "utilities/json.hpp"

#include <memory>
#include <vector>

namespace wolkabout
{
using json = nlohmann::json;
/*** SENSOR READING ***/
void to_json(json& j, const SensorReading& p)
{
    if (p.getRtc() == 0)
    {
        j = json{{"data", p.getValue()}};
    }
    else
    {
        j = json{{"utc", p.getRtc()}, {"data", p.getValue()}};
    }
}

void to_json(json& j, const std::shared_ptr<SensorReading>& p)
{
    if (p->getRtc() == 0)
    {
        j = json{{"data", p->getValue()}};
    }
    else
    {
        j = json{{"utc", p->getRtc()}, {"data", p->getValue()}};
    }
}
/*** SENSOR READING ***/

/*** ALARM ***/
void to_json(json& j, const Alarm& p)
{
    if (p.getRtc() == 0)
    {
        j = json{{"data", p.getValue()}};
    }
    else
    {
        j = json{{"utc", p.getRtc()}, {"data", p.getValue()}};
    }
}

void to_json(json& j, const std::shared_ptr<Alarm>& p)
{
    if (p->getRtc() == 0)
    {
        j = json{{"data", p->getValue()}};
    }
    else
    {
        j = json{{"utc", p->getRtc()}, {"data", p->getValue()}};
    }
}
/*** ALARM ***/

/*** ACTUATOR STATUS ***/
void to_json(json& j, const ActuatorStatus& p)
{
    const std::string status = [&]() -> std::string {
        if (p.getState() == ActuatorStatus::State::READY)
        {
            return "READY";
        }
        else if (p.getState() == ActuatorStatus::State::BUSY)
        {
            return "BUSY";
        }
        else if (p.getState() == ActuatorStatus::State::ERROR)
        {
            return "ERROR";
        }

        return "ERROR";
    }();

    j = json{{"status", status}, {"value", p.getValue()}};
}

void to_json(json& j, const std::shared_ptr<ActuatorStatus>& p)
{
    const std::string status = [&]() -> std::string {
        if (p->getState() == ActuatorStatus::State::READY)
        {
            return "READY";
        }
        else if (p->getState() == ActuatorStatus::State::BUSY)
        {
            return "BUSY";
        }
        else if (p->getState() == ActuatorStatus::State::ERROR)
        {
            return "ERROR";
        }

        return "ERROR";
    }();

    j = json{{"status", status}, {"value", p->getValue()}};
}
/*** ACTUATOR STATUS ***/

std::shared_ptr<OutboundMessage> OutboundMessageFactory::make(
  const std::string& deviceKey, std::vector<std::shared_ptr<SensorReading>> sensorReadings)
{
    if (sensorReadings.size() == 0)
    {
        return nullptr;
    }

    const json jPayload(sensorReadings);
    const std::string payload = jPayload.dump();
    const std::string topic = SENSOR_READINGS_TOPIC_ROOT + deviceKey + "/" + sensorReadings.front()->getReference();

    return std::make_shared<OutboundMessage>(payload, topic, sensorReadings.size());
}

std::shared_ptr<OutboundMessage> OutboundMessageFactory::make(const std::string& deviceKey,
                                                              std::vector<std::shared_ptr<Alarm>> alarms)
{
    if (alarms.size() == 0)
    {
        return nullptr;
    }

    const json jPayload(alarms);
    const std::string payload = jPayload.dump();
    const std::string topic = ALARMS_TOPIC_ROOT + deviceKey + "/" + alarms.front()->getReference();

    return std::make_shared<OutboundMessage>(payload, topic, alarms.size());
}

std::shared_ptr<OutboundMessage> OutboundMessageFactory::make(
  const std::string& deviceKey, std::vector<std::shared_ptr<ActuatorStatus>> actuatorStatuses)
{
    if (actuatorStatuses.size() == 0)
    {
        return nullptr;
    }

    const json jPayload(actuatorStatuses.front());
    const std::string payload = jPayload.dump();
    const std::string topic = ACTUATOR_STATUS_TOPIC_TOOT + deviceKey + "/" + actuatorStatuses.front()->getReference();

    /* Currently supported protocol (JSON_SINGLE) allows only 1 ActuatorStatus per OutboundMessage, hence 'magic' number
     * 1 below */
    return std::make_shared<OutboundMessage>(payload, topic, 1);
}
}
