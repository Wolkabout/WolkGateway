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

#include "Timer.h"

namespace wolkabout
{
Timer::Timer() : m_isRunning(false) {}

Timer::~Timer()
{
    stop();
}

void Timer::start(std::chrono::milliseconds interval, std::function<void()> callback)
{
    if (m_isRunning)
    {
        return;
    }

    m_isRunning = true;

    auto thread = [=] {
        std::unique_lock<std::mutex> lock{m_lock};
        m_condition.wait_for(lock, interval, [=] { return !m_isRunning; });

        // no callback if stopped
        if (m_isRunning && callback)
        {
            callback();
        }

        m_isRunning = false;
    };

    m_worker.reset(new std::thread(thread));
}

void Timer::run(std::chrono::milliseconds interval, std::function<void()> callback)
{
    if (m_isRunning)
    {
        return;
    }

    m_isRunning = true;

    auto thread = [=] {
        while (m_isRunning)
        {
            std::unique_lock<std::mutex> lock{m_lock};
            m_condition.wait_for(lock, interval, [=] { return !m_isRunning; });

            // no callback if stopped
            if (m_isRunning && callback)
            {
                callback();
            }
        }
    };

    m_worker.reset(new std::thread(thread));
}

void Timer::stop()
{
    m_isRunning = false;
    m_condition.notify_all();

    if (m_worker && m_worker->joinable())
    {
        m_worker->join();
    }
}

bool Timer::running() const
{
    return m_isRunning || (m_worker && m_worker->joinable());
}

}    // namespace wolkabout
