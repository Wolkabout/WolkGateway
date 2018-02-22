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

#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace wolkabout
{
class CommandBuffer
{
public:
    using Command = std::function<void()>;

    CommandBuffer();
    virtual ~CommandBuffer();

    void pushCommand(std::shared_ptr<Command> command);

private:
    std::shared_ptr<Command> popCommand();

    bool empty() const;

    void switchBuffers();

    void notify();

    void processCommands();

    void run();

    mutable std::mutex m_lock;

    std::queue<std::shared_ptr<Command>> m_pushCommandQueue;
    std::queue<std::shared_ptr<Command>> m_popCommandQueue;

    std::condition_variable m_condition;

    std::atomic_bool m_isRunning;
    std::unique_ptr<std::thread> m_worker;
};
}    // namespace wolkabout

#endif
