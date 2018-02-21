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

#include "PublishingService.h"
#include "persistence/Persistence.h"
#include "connectivity/ConnectivityService.h"
#include "model/Message.h"
#include "utilities/Logger.h"

namespace wolkabout
{
PublishingService::PublishingService(std::shared_ptr<ConnectivityService> connectivityService, std::unique_ptr<Persistence> persistence) :
	m_connectivityService{std::move(connectivityService)},
	m_persistence{std::move(persistence)},
	m_worker{new std::thread(&PublishingService::run, this)}
{
}

PublishingService::~PublishingService()
{
	m_run = false;
	m_condition.notify_one();
	m_worker->join();
}

void PublishingService::addMessage(std::shared_ptr<Message> message)
{
	LOG(DEBUG) << "Message added " << message->getTopic() << " " << message->getContent();
	m_persistence->push(message);
	m_condition.notify_one();
}

void PublishingService::connected()
{
	m_connected = true;
	m_condition.notify_one();
}

void PublishingService::disconnected()
{
	m_connected = false;
}

void PublishingService::run()
{
	while(m_run)
	{
		while(m_connected && !m_persistence->empty())
		{
			const auto message = m_persistence->front();
			if(m_connectivityService->publish(message))
			{
				m_persistence->pop();
			}
		}

		std::unique_lock<std::mutex> locker{m_lock};
		m_condition.wait(locker);
	}
}
}
