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

#ifndef TIMER_H
#define TIMER_H

#include <thread>
#include <atomic>
#include <functional>
#include <condition_variable>
#include <memory>

namespace wolkabout
{

class Timer
{
public:
	Timer();
	~Timer();

	void start(unsigned intervalMsec, std::function<void()> callback);
	void stop();
	bool running() const;

private:
	std::atomic_bool m_isRunning;
	std::mutex m_lock;
	std::condition_variable m_condition;
	std::unique_ptr<std::thread> m_worker;
};

}

#endif // TIMER_H
