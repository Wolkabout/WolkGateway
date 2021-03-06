/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#include "ConnectionStatusListener.h"
#include "OutboundMessageHandler.h"
#include "core/utilities/Buffer.h"
#include "persistence/GatewayPersistence.h"

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

namespace wolkabout
{
class ConnectivityService;

class PublishingService : public OutboundMessageHandler, public ConnectionStatusListener
{
public:
    PublishingService(ConnectivityService& connectivityService, std::shared_ptr<GatewayPersistence> persistence);
    ~PublishingService();

    void addMessage(std::shared_ptr<Message> message) override;

    void connected() override;
    void disconnected() override;

private:
    class State
    {
    public:
        State(PublishingService& service);
        virtual ~State() = default;
        virtual void run() = 0;

    protected:
        PublishingService& m_service;
    };

    class DisconnectedState : public State
    {
    public:
        using State::State;
        void run() override;
    };

    class ConnectedState : public State
    {
    public:
        using State::State;
        void run() override;
    };

    void run();

    ConnectivityService& m_connectivityService;
    std::shared_ptr<GatewayPersistence> m_persistence;
    ConnectedState m_connectedState;
    DisconnectedState m_disconnectedState;
    std::atomic<State*> m_currentState;

    std::atomic_bool m_connected;

    Buffer<std::shared_ptr<Message>> m_buffer;

    std::atomic_bool m_run;
    std::unique_ptr<std::thread> m_worker;
};
}    // namespace wolkabout

#endif
