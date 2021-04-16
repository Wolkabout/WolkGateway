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

#include "core/connectivity/ConnectivityService.h"
#include "core/model/Message.h"
#include "core/utilities/Logger.h"

namespace wolkabout
{
PublishingService::PublishingService(ConnectivityService& connectivityService,
                                     std::shared_ptr<GatewayPersistence> persistence)
: m_connectivityService{connectivityService}
, m_persistence{std::move(persistence)}
, m_connectedState{*this}
, m_disconnectedState{*this}
, m_currentState{&m_disconnectedState}
, m_connected{false}
, m_run{true}
, m_worker{new std::thread(&PublishingService::run, this)}
{
}

PublishingService::~PublishingService()
{
    m_run = false;
    m_buffer.notify();
    m_worker->join();
}

void PublishingService::addMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << "PublishingService: Message added. Channel: '" << message->getChannel() << "' Payload: '"
               << message->getContent() << "'";
    m_buffer.push(std::move(message));
}

void PublishingService::connected()
{
    m_connected = true;
    m_currentState = &m_connectedState;
    m_buffer.notify();
}

void PublishingService::disconnected()
{
    m_connected = false;
    m_currentState = &m_disconnectedState;
    m_buffer.notify();
}

void PublishingService::run()
{
    while (m_run)
    {
        auto state = m_currentState.load();
        state->run();
    }

    m_buffer.notify();
}

PublishingService::State::State(PublishingService& service) : m_service{service} {}

void PublishingService::DisconnectedState::run()
{
    while (m_service.m_run && !m_service.m_connected && !m_service.m_buffer.isEmpty())
    {
        const auto message = m_service.m_buffer.pop();
        if (!m_service.m_persistence->push(message))
        {
            LOG(ERROR) << "Failed to persist message";
        }
    }

    m_service.m_buffer.swapBuffers();
}

void PublishingService::ConnectedState::run()
{
    while (m_service.m_run && m_service.m_connected && !m_service.m_buffer.isEmpty())
    {
        const auto message = m_service.m_buffer.pop();
        if (m_service.m_connectivityService.publish(message))
        {
            m_service.m_persistence->pop();
        }
        else
        {
            LOG(ERROR) << "Failed to publish message";
        }
    }

    // publish persisted unitl new message arrives
    while (m_service.m_run && m_service.m_connected && !m_service.m_persistence->empty() &&
           m_service.m_buffer.isEmpty())
    {
        const auto message = m_service.m_persistence->front();
        if (m_service.m_connectivityService.publish(message))
        {
            m_service.m_persistence->pop();
        }
        else
        {
            LOG(ERROR) << "Failed to publish message";
        }
    }

    m_service.m_buffer.swapBuffers();
}

}    // namespace wolkabout
