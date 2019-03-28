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

#include "service/GatewayDataService.h"
#include "OutboundMessageHandler.h"
#include "connectivity/ConnectivityService.h"
#include "model/ActuatorGetCommand.h"
#include "model/ActuatorSetCommand.h"
#include "model/ConfigurationSetCommand.h"
#include "model/Message.h"
#include "model/SensorReading.h"
#include "persistence/Persistence.h"
#include "protocol/DataProtocol.h"
#include "utilities/Logger.h"

#include <algorithm>
#include <cassert>

namespace wolkabout
{
GatewayDataService::GatewayDataService(std::string deviceKey, DataProtocol& protocol, Persistence& persistence,
                                       OutboundMessageHandler& outboundMessageHandler,
                                       const ActuatorSetHandler& actuatorSetHandler,
                                       const ActuatorGetHandler& actuatorGetHandler,
                                       const ConfigurationSetHandler& configurationSetHandler,
                                       const ConfigurationGetHandler& configurationGetHandler)
: m_deviceKey{std::move(deviceKey)}
, m_protocol{protocol}
, m_persistence{persistence}
, m_outboundMessageHandler{outboundMessageHandler}
, m_actuatorSetHandler{actuatorSetHandler}
, m_actuatorGetHandler{actuatorGetHandler}
, m_configurationSetHandler{configurationSetHandler}
, m_configurationGetHandler{configurationGetHandler}
{
}

void GatewayDataService::messageReceived(std::shared_ptr<Message> message)
{
    assert(message);

    const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(message->getChannel());
    if (deviceKey.empty())
    {
        LOG(WARN) << "Unable to extract device key from channel: " << message->getChannel();
        return;
    }

    if (deviceKey != m_deviceKey)
    {
        LOG(WARN) << "Device key mismatch: " << message->getChannel();
        return;
    }

    if (m_protocol.isActuatorGetMessage(*message))
    {
        auto command = m_protocol.makeActuatorGetCommand(*message);
        if (!command)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        if (m_actuatorGetHandler)
        {
            m_actuatorGetHandler(command->getReference());
        }
    }
    else if (m_protocol.isActuatorSetMessage(*message))
    {
        auto command = m_protocol.makeActuatorSetCommand(*message);
        if (!command)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        if (m_actuatorSetHandler)
        {
            m_actuatorSetHandler(command->getReference(), command->getValue());
        }
    }
    else if (m_protocol.isConfigurationGetMessage(*message))
    {
        if (m_configurationGetHandler)
        {
            m_configurationGetHandler();
        }
    }
    else if (m_protocol.isConfigurationSetMessage(*message))
    {
        auto command = m_protocol.makeConfigurationSetCommand(*message);
        if (!command)
        {
            LOG(WARN) << "Unable to parse message contents: " << message->getContent();
            return;
        }

        if (m_configurationSetHandler)
        {
            m_configurationSetHandler(*command);
        }
    }
    else
    {
        LOG(WARN) << "Unable to parse message channel: " << message->getChannel();
    }
}

const Protocol& GatewayDataService::getProtocol()
{
    return m_protocol;
}

void GatewayDataService::addSensorReading(const std::string& reference, const std::string& value,
                                          unsigned long long int rtc)
{
    auto sensorReading = std::make_shared<SensorReading>(value, reference, rtc);

    m_persistence.putSensorReading(reference, sensorReading);
}

void GatewayDataService::addSensorReading(const std::string& reference, const std::vector<std::string>& values,
                                          unsigned long long int rtc)
{
    auto sensorReading = std::make_shared<SensorReading>(values, reference, rtc);

    m_persistence.putSensorReading(reference, sensorReading);
}

void GatewayDataService::addAlarm(const std::string& reference, bool active, unsigned long long int rtc)
{
    auto alarm = std::make_shared<Alarm>(active, reference, rtc);

    m_persistence.putAlarm(reference, alarm);
}

void GatewayDataService::addActuatorStatus(const std::string& reference, const std::string& value,
                                           ActuatorStatus::State state)
{
    auto actuatorStatusWithRef = std::make_shared<ActuatorStatus>(value, reference, state);

    m_persistence.putActuatorStatus(reference, actuatorStatusWithRef);
}

void GatewayDataService::addConfiguration(const std::vector<ConfigurationItem>& configuration)
{
    auto conf = std::make_shared<std::vector<ConfigurationItem>>(configuration);

    m_persistence.putConfiguration(m_deviceKey, conf);
}

void GatewayDataService::publishSensorReadings()
{
    for (const auto& key : m_persistence.getSensorReadingsKeys())
    {
        publishSensorReadingsForPersistanceKey(key);
    }
}

void GatewayDataService::publishSensorReadingsForPersistanceKey(const std::string& persistanceKey)
{
    const auto sensorReadings = m_persistence.getSensorReadings(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);

    if (sensorReadings.empty())
    {
        return;
    }

    const std::shared_ptr<Message> outboundMessage = m_protocol.makeMessage(m_deviceKey, sensorReadings);

    if (!outboundMessage)
    {
        LOG(ERROR) << "Unable to create message from readings: " << persistanceKey;
        m_persistence.removeSensorReadings(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);
        return;
    }

    m_outboundMessageHandler.addMessage(outboundMessage);

    m_persistence.removeSensorReadings(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);

    publishSensorReadingsForPersistanceKey(persistanceKey);
}

void GatewayDataService::publishAlarms()
{
    for (const auto& key : m_persistence.getAlarmsKeys())
    {
        publishAlarmsForPersistanceKey(key);
    }
}

void GatewayDataService::publishAlarmsForPersistanceKey(const std::string& persistanceKey)
{
    const auto alarms = m_persistence.getAlarms(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);

    if (alarms.empty())
    {
        return;
    }

    const std::shared_ptr<Message> outboundMessage = m_protocol.makeMessage(m_deviceKey, alarms);

    if (!outboundMessage)
    {
        LOG(ERROR) << "Unable to create message from alarms: " << persistanceKey;
        m_persistence.removeAlarms(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);
        return;
    }

    m_outboundMessageHandler.addMessage(outboundMessage);

    m_persistence.removeAlarms(persistanceKey, PUBLISH_BATCH_ITEMS_COUNT);

    publishAlarmsForPersistanceKey(persistanceKey);
}

void GatewayDataService::publishActuatorStatuses()
{
    for (const auto& key : m_persistence.getActuatorStatusesKeys())
    {
        publishActuatorStatusesForPersistanceKey(key);
    }
}

void GatewayDataService::publishActuatorStatusesForPersistanceKey(const std::string& persistanceKey)
{
    const auto actuatorStatus = m_persistence.getActuatorStatus(persistanceKey);

    if (!actuatorStatus)
    {
        return;
    }

    const std::shared_ptr<Message> outboundMessage = m_protocol.makeMessage(m_deviceKey, {actuatorStatus});

    if (!outboundMessage)
    {
        LOG(ERROR) << "Unable to create message from actuator status: " << persistanceKey;
        m_persistence.removeActuatorStatus(persistanceKey);
        return;
    }

    m_outboundMessageHandler.addMessage(outboundMessage);

    m_persistence.removeActuatorStatus(persistanceKey);
}

void GatewayDataService::publishConfiguration()
{
    publishConfigurationForPersistanceKey(m_deviceKey);
}

void GatewayDataService::publishConfigurationForPersistanceKey(const std::string& persistanceKey)
{
    const auto configuration = m_persistence.getConfiguration(persistanceKey);

    if (!configuration)
    {
        return;
    }

    const std::shared_ptr<Message> outboundMessage = m_protocol.makeMessage(persistanceKey, *configuration);

    if (!outboundMessage)
    {
        LOG(ERROR) << "Unable to create message from configuration: " << persistanceKey;
        m_persistence.removeConfiguration(persistanceKey);
        return;
    }

    m_outboundMessageHandler.addMessage(outboundMessage);

    m_persistence.removeConfiguration(persistanceKey);
}
}    // namespace wolkabout
