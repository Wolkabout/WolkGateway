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

#ifndef PUBLISHINGSERVICE_H
#define PUBLISHINGSERVICE_H

#include "ActuatorStatus.h"
#include "MqttService.h"
#include "Reading.h"
#include "ReadingsBuffer.h"
#include "SensorReading.h"

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace wolkabout
{
class PublishingService
{
public:
    PublishingService(std::shared_ptr<MqttService> mqttService, std::shared_ptr<ReadingBuffer> readingBuffer,
                      std::string deviceKey,
                      std::chrono::milliseconds publishInterval = std::chrono::milliseconds(200));

    virtual ~PublishingService() = default;

    void start();
    void stop();

    void flush();

private:
    void run();

    void publishReadings();

    class ReadingPublisherVisitor : public ReadingVisitor
    {
    public:
        ReadingPublisherVisitor(MqttService& mqttService, std::string& deviceKey)
        : m_mqttService(mqttService), m_devicekey(deviceKey)
        {
        }

        ~ReadingPublisherVisitor() = default;

        void visit(ActuatorStatus& actuatorStatus) override;
        void visit(Alarm& event) override;
        void visit(SensorReading& sensorReading) override;

    private:
        MqttService& m_mqttService;
        std::string& m_devicekey;
    };

    std::shared_ptr<MqttService> m_mqttService;
    std::shared_ptr<ReadingBuffer> m_readingBuffer;

    std::string m_deviceKey;

    std::chrono::milliseconds m_publishInterval;

    std::atomic_bool m_isRunning;
    std::atomic_bool m_shouldWaitForPublish;
    std::unique_ptr<std::thread> m_worker;
};
}

#endif
