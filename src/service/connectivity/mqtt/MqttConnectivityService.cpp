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

#include "service/connectivity/mqtt/MqttConnectivityService.h"
#include "model/ActuatorCommand.h"
#include "model/Device.h"
#include "model/Reading.h"
#include "service/connectivity/ConnectivityService.h"
#include "service/connectivity/mqtt/JsonDtoParser.h"
#include "service/connectivity/mqtt/MqttClient.h"
#include "service/connectivity/mqtt/PahoMqttClient.h"
#include "service/connectivity/mqtt/dto/ActuatorCommandDto.h"
#include "service/connectivity/mqtt/dto/ActuatorStatusDto.h"
#include "service/connectivity/mqtt/dto/AlarmDto.h"
#include "service/connectivity/mqtt/dto/SensorReadingDto.h"

#include <atomic>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace wolkabout
{
MqttConnectivityService::MqttConnectivityService(std::shared_ptr<MqttClient> mqttClient, Device device,
                                                 std::string host)
: m_device(std::move(device))
, m_host(std::move(host))
, m_subscriptionList({})
, m_connected(false)
, m_mqttClient(mqttClient)
{
    for (const std::string& actuatorReference : m_device.getActuatorReferences())
    {
        std::stringstream topic("");
        topic << TOPIC_ROOT_ACTUATION_REQUEST << m_device.getDeviceKey() << "/" << actuatorReference;
        m_subscriptionList.emplace_back(topic.str());
    }

    m_mqttClient->onMessageReceived([this](std::string topic, std::string message) -> void {
        const size_t referencePosition = topic.find_last_of('/');
        if (referencePosition == std::string::npos)
        {
            return;
        }

        ActuatorCommandDto actuatorCommandDto;
        if (!JsonDtoParser::fromJson(message, actuatorCommandDto))
        {
            return;
        }

        /* Workaround for:
         * Bool actuation from mobile clients sends '0' instead of 'false', and '1', instead of 'true'
         */
        if (actuatorCommandDto.getValue() == "0" || actuatorCommandDto.getValue() == "1")
        {
            actuatorCommandDto =
              ActuatorCommandDto(actuatorCommandDto.getType(), actuatorCommandDto.getValue() == "0" ? "false" : "true");
        }

        const std::string reference = topic.substr(referencePosition + 1);
        ConnectivityService::invokeListener(
          ActuatorCommand(actuatorCommandDto.getType(), reference, actuatorCommandDto.getValue()));
    });
}

bool MqttConnectivityService::connect()
{
    m_mqttClient->setLastWill(TOPIC_ROOT_LAST_WILL + m_device.getDeviceKey(), "Gone offline");
    bool isConnected = m_mqttClient->connect(m_device.getDeviceKey(), m_device.getDevicePassword(), TRUST_STORE, m_host,
                                             m_device.getDeviceKey());
    if (isConnected)
    {
        for (const std::string& topic : m_subscriptionList)
        {
            m_mqttClient->subscribe(topic);
        }
    }

    return isConnected;
}

void MqttConnectivityService::disconnect()
{
    m_mqttClient->disconnect();
}

bool MqttConnectivityService::isConnected()
{
    return m_mqttClient->isConnected();
}

bool MqttConnectivityService::publish(std::shared_ptr<Reading> reading)
{
    bool isPublished = false;
    ReadingPublisherVisitor readingPublisher(*m_mqttClient, m_device, isPublished);
    reading->acceptVisit(readingPublisher);

    return isPublished;
}

void MqttConnectivityService::ReadingPublisherVisitor::visit(SensorReading& sensorReading)
{
    std::string topic = TOPIC_ROOT_SENSOR_READING + m_device.getDeviceKey() + "/" + sensorReading.getReference();
    std::string messagePayload = JsonDtoParser::toJson(sensorReading);

    m_isPublished = m_mqttClient.publish(topic, messagePayload);
}

void MqttConnectivityService::ReadingPublisherVisitor::visit(ActuatorStatus& actuatorStatus)
{
    std::string topic = TOPIC_ROOT_ACTUATOR_STATUS + m_device.getDeviceKey() + "/" + actuatorStatus.getReference();
    std::string messagePayload = JsonDtoParser::toJson(ActuatorStatusDto(actuatorStatus));

    m_isPublished = m_mqttClient.publish(topic, messagePayload);
}

void MqttConnectivityService::ReadingPublisherVisitor::visit(Alarm& alarm)
{
    std::string topic = TOPIC_ROOT_ALARM + m_device.getDeviceKey() + "/" + alarm.getReference();
    std::string messagePayload = JsonDtoParser::toJson(AlarmDto(alarm));

    m_isPublished = m_mqttClient.publish(topic, messagePayload);
}
}
