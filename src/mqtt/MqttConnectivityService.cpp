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

#include "MqttConnectivityService.h"
#include "ActuatorCommand.h"
#include "ConnectivityService.h"
#include "Device.h"
#include "JsonParser.h"
#include "MqttClient.h"
#include "PahoMqttClient.h"
#include "Reading.h"

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

        ActuatorCommand actuatorCommand;
        JsonParser::fromJson(message, actuatorCommand);

        const std::string reference = topic.substr(referencePosition + 1);
        if (auto listener = m_listener.lock())
        {
            listener->actuatorCommandReceived(actuatorCommand, reference);
        }
    });
}

bool MqttConnectivityService::connect()
{
    m_mqttClient->setLastWill(TOPIC_ROOT_LAST_WILL + m_device.getDeviceKey(), "Gone offline");
    return m_mqttClient->connect(m_device.getDeviceKey(), m_device.getDevicePassword(), TRUST_STORE, m_host,
                                 m_device.getDeviceKey());
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
    std::string messagePayload = JsonParser::toJson(sensorReading);

    m_isPublished = m_mqttClient.publish(topic, messagePayload);
}

void MqttConnectivityService::ReadingPublisherVisitor::visit(ActuatorStatus& actuatorStatus)
{
    std::string topic = TOPIC_ROOT_ACTUATOR_STATUS + m_device.getDeviceKey() + "/" + actuatorStatus.getReference();
    std::string messagePayload = JsonParser::toJson(actuatorStatus);

    m_isPublished = m_mqttClient.publish(topic, messagePayload);
}

void MqttConnectivityService::ReadingPublisherVisitor::visit(Alarm& event)
{
    std::string topic = TOPIC_ROOT_ALARM + m_device.getDeviceKey() + "/" + event.getReference();
    std::string messagePayload = JsonParser::toJson(event);

    m_isPublished = m_mqttClient.publish(topic, messagePayload);
}
}
