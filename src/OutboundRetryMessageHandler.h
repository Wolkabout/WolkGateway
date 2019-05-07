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

#ifndef OUTBOUNDRETRYMESSAGEHANDLER_H
#define OUTBOUNDRETRYMESSAGEHANDLER_H

#include "utilities/Timer.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <tuple>

namespace wolkabout
{
class Message;
class OutboundMessageHandler;

struct RetryMessageStruct
{
    std::shared_ptr<Message> message;
    std::string responseChannel;
    std::function<void(std::shared_ptr<Message>)> onFail;
    short retryCount;
    std::chrono::milliseconds retryInterval;
};

class OutboundRetryMessageHandler
{
public:
    explicit OutboundRetryMessageHandler(OutboundMessageHandler& messageHandler);
    ~OutboundRetryMessageHandler();

    void addMessage(RetryMessageStruct msg);
    void messageReceived(std::shared_ptr<Message> message);

private:
    void clearTimers();
    void notifyCleanup();
    unsigned long long getUniqueId();

    OutboundMessageHandler& m_messageHandler;

    std::map<unsigned long long, std::tuple<RetryMessageStruct, std::unique_ptr<Timer>, short, bool>> m_messages;

    std::atomic_bool m_run;
    std::condition_variable m_condition;
    std::mutex m_mutex;
    std::thread m_garbageCollector;
};
}    // namespace wolkabout

#endif    // OUTBOUNDRETRYMESSAGEHANDLER_H
