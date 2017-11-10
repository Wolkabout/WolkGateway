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

#ifndef PUBLISHSERVICE_H
#define PUBLISHSERVICE_H

#include "model/ActuatorStatus.h"
#include "model/Reading.h"
#include "model/SensorReading.h"
#include "service/connectivity/ConnectivityService.h"
#include "service/persist/PersistService.h"
#include "service/publish/ReadingsBuffer.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <string>
#include <thread>

namespace wolkabout
{
class PublishService
{
public:
    PublishService(std::shared_ptr<ConnectivityService> connectivityService,
                   std::shared_ptr<PersistService> persistService, std::chrono::milliseconds publishInterval);

    virtual ~PublishService() = default;

    void start();
    void stop();

    void flush();

    void addReading(std::unique_ptr<Reading> reading);

private:
    void run();

    void publishOrPersistReadings();

    void sleepUntilNextPublishCycle();

    std::shared_ptr<ConnectivityService> m_connectivityService;

    std::shared_ptr<PersistService> m_persistService;

    std::chrono::milliseconds m_publishInterval;

    std::unique_ptr<ReadingBuffer> m_readingBuffer;

    std::atomic_bool m_isRunning;
    std::condition_variable m_flushReadings;

    std::unique_ptr<std::thread> m_worker;
};
}

#endif
