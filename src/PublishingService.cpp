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

#include "PublishingService.h"
#include "ActuatorStatus.h"
#include "JsonParser.h"
#include "MqttService.h"
#include "Reading.h"
#include "ReadingsBuffer.h"
#include "SensorReading.h"

#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace wolkabout
{
PublishingService::PublishingService(std::shared_ptr<MqttService> mqttService,
                                     std::shared_ptr<ReadingBuffer> readingBuffer, std::string deviceKey,
                                     std::chrono::milliseconds publishInterval)
: m_mqttService(mqttService)
, m_readingBuffer(readingBuffer)
, m_deviceKey(std::move(deviceKey))
, m_publishInterval(std::move(publishInterval))
, m_isRunning(false)
, m_shouldWaitForPublish(false)
{
}

void PublishingService::start()
{
    m_isRunning = true;
    m_worker = std::unique_ptr<std::thread>(new std::thread(&PublishingService::run, this));
}

void PublishingService::stop()
{
    m_isRunning = false;
    m_worker->join();
}

void PublishingService::flush()
{
    publishReadings();
}

void PublishingService::run()
{
    while (m_isRunning)
    {
        if (m_mqttService->isConnected())
        {
            publishReadings();
        }
        else
        {
            m_mqttService->connect();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(m_publishInterval));
    }
}

void PublishingService::publishReadings()
{
    if (m_readingBuffer->hasReadings())
    {
        ReadingPublisherVisitor readingPublisher(*m_mqttService.get(), m_deviceKey);
        for (const std::unique_ptr<Reading>& reading : m_readingBuffer->getReadings())
        {
            reading->acceptVisit(readingPublisher);
        }
    }
}

void PublishingService::ReadingPublisherVisitor::visit(SensorReading& sensorReading)
{
    std::string topic = "readings/" + m_devicekey + "/" + sensorReading.getReference();
    std::string messagePayload = JsonParser::toJson(sensorReading);

    m_mqttService.publish(topic, messagePayload);
}

void PublishingService::ReadingPublisherVisitor::visit(ActuatorStatus& actuatorStatus)
{
    std::string topic = "actuators/status/" + m_devicekey + "/" + actuatorStatus.getReference();
    std::string messagePayload = JsonParser::toJson(actuatorStatus);

    m_mqttService.publish(topic, messagePayload);
}

void PublishingService::ReadingPublisherVisitor::visit(Alarm& event)
{
    std::string topic = "events/" + m_devicekey + "/" + event.getReference();
    std::string messagePayload = JsonParser::toJson(event);

    m_mqttService.publish(topic, messagePayload);
}
}
