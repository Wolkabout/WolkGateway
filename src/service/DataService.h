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

#ifndef DATASERVICE_H
#define DATASERVICE_H

#include "model/Message.h"
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class SensorReading;
class Alarm;
class ActuatorStatus;
class MessageFactory;
class OutboundMessageHandler;
class ActuatorCommandListener;

class DataService
{
public:
    DataService(const std::string& gatewayKey, std::unique_ptr<MessageFactory> protocol,
                std::shared_ptr<OutboundMessageHandler> outboundWolkaboutMessageHandler,
                std::shared_ptr<OutboundMessageHandler> outboundModuleMessageHandler,
                std::weak_ptr<ActuatorCommandListener> actuationHandler);

    void handleSensorReading(Message reading);
    void handleAlarm(Message alarm);
    void handleActuatorSetCommand(Message command);
    void handleActuatorGetCommand(Message command);
    void handleActuatorStatus(Message status);

    void addSensorReadings(std::vector<std::shared_ptr<SensorReading>> sensorReadings);
    void addAlarms(std::vector<std::shared_ptr<Alarm>> alarms);
    void addActuatorStatus(std::shared_ptr<ActuatorStatus> actuatorStatus);

private:
    void routeModuleMessage(const Message& message, const std::string& topicRoot);
    void routeWolkaboutMessage(const Message& message);

    const std::string m_gatewayKey;
    std::unique_ptr<MessageFactory> m_protocol;

    std::shared_ptr<OutboundMessageHandler> m_outboundWolkaboutMessageHandler;
    std::shared_ptr<OutboundMessageHandler> m_outboundModuleMessageHandler;

    std::weak_ptr<ActuatorCommandListener> m_actuationHandler;
};
}    // namespace wolkabout

#endif
