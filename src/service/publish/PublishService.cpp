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

#include "service/publish/PublishService.h"
#include "model/ActuatorStatus.h"
#include "model/Reading.h"
#include "model/SensorReading.h"
#include "service/publish/ReadingsBuffer.h"

#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace wolkabout
{
PublishService::PublishService(std::shared_ptr<ConnectivityService> connectivityService,
                               std::shared_ptr<PersistService> persistService,
                               std::chrono::milliseconds publishInterval)
: m_connectivityService(connectivityService)
, m_persistService(persistService)
, m_publishInterval(std::move(publishInterval))
, m_readingBuffer(new ReadingBuffer())
, m_isRunning(false)
{
}

void PublishService::start()
{
    if (m_isRunning)
    {
        return;
    }

    m_isRunning = true;
    m_worker = std::unique_ptr<std::thread>(new std::thread(&PublishService::run, this));
}

void PublishService::stop()
{
    if (!m_isRunning)
    {
        return;
    }

    flush();

    m_isRunning = false;
    m_worker->join();
}

void PublishService::flush()
{
    m_flushReadings.notify_one();
}

void PublishService::addReading(std::unique_ptr<Reading> reading)
{
    m_readingBuffer->addReading(std::move(reading));
}

void PublishService::run()
{
    while (m_isRunning)
    {
        sleepUntilNextPublishCycle();

        if (!m_connectivityService->isConnected())
        {
            m_connectivityService->connect();
            if (!m_persistService)
            {
                continue;
            }
        }

        if (m_connectivityService->isConnected() && m_persistService)
        {
            while (m_persistService->hasPersistedReadings())
            {
                std::shared_ptr<Reading> reading = m_persistService->unpersistFirst();
                if (reading == nullptr)
                {
                    m_persistService->dropFirst();
                    continue;
                }

                if (!m_connectivityService->publish(reading))
                {
                    break;
                }

                m_persistService->dropFirst();
            }
        }

        publishOrPersistReadings();
    }

    m_connectivityService->disconnect();
}

void PublishService::publishOrPersistReadings()
{
    if (!m_readingBuffer->hasReadings())
    {
        return;
    }

    for (std::shared_ptr<Reading> reading : m_readingBuffer->getReadings())
    {
        if (!m_connectivityService->publish(reading))
        {
            if (m_persistService)
            {
                m_persistService->persist(reading);
            }
            else
            {
                m_readingBuffer->addReading(reading);
            }
        }
    }
}

void PublishService::sleepUntilNextPublishCycle()
{
    std::mutex lock;
    std::unique_lock<std::mutex> unique_lock(lock);
    m_flushReadings.wait_for(unique_lock, m_publishInterval);
}
}
