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
PublishingService::PublishingService(std::shared_ptr<ConnectivityService> connectivityService,
                                     std::shared_ptr<ReadingBuffer> readingBuffer,
                                     std::chrono::milliseconds publishInterval)
: m_connectivityService(connectivityService)
, m_readingBuffer(readingBuffer)
, m_publishInterval(std::move(publishInterval))
, m_isRunning(false)
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
    m_flushReadings.notify_one();
}

void PublishingService::run()
{
    while (m_isRunning)
    {
        if (!m_connectivityService->isConnected())
        {
            m_connectivityService->connect();
        }

        publishReadings();

        sleepUntilNextPublishCycle();
    }
}

void PublishingService::publishReadings()
{
    if (!m_readingBuffer->hasReadings())
    {
        return;
    }

    std::vector<std::shared_ptr<Reading>> readings = m_readingBuffer->getReadings();
    std::for_each(readings.begin(), readings.end(),
                  [this](std::shared_ptr<Reading> reading) -> void { publishReading(reading); });
}

void PublishingService::publishReading(std::shared_ptr<Reading> reading)
{
    m_connectivityService->publish(reading);
}

void PublishingService::sleepUntilNextPublishCycle()
{
    std::mutex lock;
    std::unique_lock<std::mutex> unique_lock(lock);
    m_flushReadings.wait_for(unique_lock, m_publishInterval);
}
}
