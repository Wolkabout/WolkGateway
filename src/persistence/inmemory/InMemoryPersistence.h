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

#ifndef INMEMORYPERSISTENCE_H
#define INMEMORYPERSISTENCE_H

#include "persistence/Persistence.h"
#include <mutex>
#include <queue>

namespace wolkabout
{
class InMemoryPersistence : public Persistence
{
public:
    bool push(std::shared_ptr<Message> message) override;
    std::shared_ptr<Message> pop() override;
    std::shared_ptr<Message> front() override;
    bool empty() const override;

private:
    mutable std::mutex m_lock;
    std::queue<std::shared_ptr<Message>> m_queue;
};
}

#endif
