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

#include "OutboundRetryMessageHandler.h"
#include "OutboundMessageHandler.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"

namespace
{
static const size_t RETRY_MESSAGE_INDEX = 0;
static const size_t TIMER_INDEX = 1;
static const size_t RETRY_COUNT_INDEX = 2;
static const size_t FLAG_INDEX = 3;
}    // namespace

namespace wolkabout
{
OutboundRetryMessageHandler::OutboundRetryMessageHandler(OutboundMessageHandler& messageHandler)
: m_messageHandler{messageHandler}, m_run{true}, m_garbageCollector(&OutboundRetryMessageHandler::clearTimers, this)
{
}

OutboundRetryMessageHandler::~OutboundRetryMessageHandler()
{
    m_run = false;
    notifyCleanup();

    if (m_garbageCollector.joinable())
    {
        m_garbageCollector.join();
    }
}

void OutboundRetryMessageHandler::addMessage(RetryMessageStruct msg)
{
    std::lock_guard<decltype(m_mutex)> lg{m_mutex};

    LOG(DEBUG) << "Adding message for retry on channel: " << msg.message->getChannel();

    // send message
    m_messageHandler.addMessage(msg.message);

    // setup retry
    const auto id = getUniqueId();
    m_messages[id] = std::make_tuple(msg, std::unique_ptr<Timer>(new Timer()), 0, false);

    auto& tup = m_messages[id];
    auto& timer = std::get<TIMER_INDEX>(tup);
    auto interval = std::get<RETRY_MESSAGE_INDEX>(tup).retryInterval;

    timer->run(interval, [=] {
        std::lock_guard<decltype(m_mutex)> l{m_mutex};
        auto it = m_messages.find(id);
        if (it == m_messages.end())
            return;

        auto& tuple = it->second;

        // skip if message is already flagged
        if (std::get<FLAG_INDEX>(tuple))
            return;

        auto& retryCount = std::get<RETRY_COUNT_INDEX>(tuple);
        ++retryCount;

        auto& retryMessage = std::get<RETRY_MESSAGE_INDEX>(tuple);

        if (retryCount > retryMessage.retryCount)
        {
            LOG(INFO) << "Retry count exceeded for message on channel: " << retryMessage.message->getChannel();

            // flag message struct for deletion
            auto& clearMessage = std::get<FLAG_INDEX>(tuple);
            clearMessage = true;

            // on fail callback
            retryMessage.onFail(retryMessage.message);

            notifyCleanup();
        }
        else
        {
            LOG(INFO) << "Retry sending message on channel: " << retryMessage.message->getChannel();
            // retry message sending
            m_messageHandler.addMessage(retryMessage.message);
        }
    });
}

void OutboundRetryMessageHandler::messageReceived(std::shared_ptr<Message> response)
{
    std::lock_guard<decltype(m_mutex)> lg{m_mutex};

    for (auto& kvp : m_messages)
    {
        auto& tuple = kvp.second;
        const auto& retryMessage = std::get<RETRY_MESSAGE_INDEX>(tuple);
        if (StringUtils::mqttTopicMatch(response->getChannel(), retryMessage.responseChannel))
        {
            LOG(DEBUG) << "Response received on channel " << retryMessage.responseChannel
                       << ", for message on channel: " << retryMessage.message->getChannel();

            // flag message struct for deletion
            auto& clearMessage = std::get<FLAG_INDEX>(tuple);
            clearMessage = true;

            // stop retry timer
            auto& timer = std::get<TIMER_INDEX>(tuple);
            timer->stop();

            notifyCleanup();
        }
    }
}

void OutboundRetryMessageHandler::clearTimers()
{
    while (m_run)
    {
        std::unique_lock<decltype(m_mutex)> lg{m_mutex};

        for (auto it = m_messages.begin(); it != m_messages.end();)
        {
            auto& tuple = it->second;
            auto& clearMessage = std::get<FLAG_INDEX>(tuple);

            if (clearMessage)
            {
                LOG(DEBUG) << "Removing message from retry queue: "
                           << std::get<RETRY_MESSAGE_INDEX>(tuple).message->getChannel();
                // removed flagged messages
                auto& timer = std::get<TIMER_INDEX>(tuple);
                timer->stop();
                it = m_messages.erase(it);
            }
            else
            {
                ++it;
            }
        }

        lg.unlock();

        static std::mutex cvMutex;
        std::unique_lock<std::mutex> lock{cvMutex};
        m_condition.wait(lock);
    }
}

void OutboundRetryMessageHandler::notifyCleanup()
{
    m_condition.notify_one();
}

unsigned long long OutboundRetryMessageHandler::getUniqueId()
{
    static unsigned long long id = 0;
    return ++id;
}

}    // namespace wolkabout
