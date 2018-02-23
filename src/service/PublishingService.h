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

#ifndef PUBLISHINGSERVICE_H
#define PUBLISHINGSERVICE_H

#include "OutboundMessageHandler.h"
#include "ConnectionStatusListener.h"
#include <memory>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace wolkabout
{
class ConnectivityService;
class Persistence;

class PublishingService: public OutboundMessageHandler, public ConnectionStatusListener
{
public:
	PublishingService(std::shared_ptr<ConnectivityService> connectivityService, std::unique_ptr<Persistence> persistence);
	~PublishingService();

	void addMessage(std::shared_ptr<Message> message) override;

	void connected() override;
	void disconnected() override;

private:
	void run();

	std::shared_ptr<ConnectivityService> m_connectivityService;
	std::unique_ptr<Persistence> m_persistence;

	std::atomic_bool m_connected;

	std::atomic_bool m_run;
	std::mutex m_lock;
	std::unique_ptr<std::thread> m_worker;
	std::condition_variable m_condition;
};
}

#endif
