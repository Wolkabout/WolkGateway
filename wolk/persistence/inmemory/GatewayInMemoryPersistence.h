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

#ifndef GATEWAYINMEMORYPERSISTENCE_H
#define GATEWAYINMEMORYPERSISTENCE_H

#include "wolk/persistence/GatewayPersistence.h"

#include <memory>
#include <mutex>
#include <queue>

namespace wolkabout
{
class GatewayInMemoryPersistence : public GatewayPersistence
{
public:
    bool push(std::shared_ptr<Message> message) override;
    void pop() override;
    std::shared_ptr<Message> front() override;
    bool empty() const override;

private:
    mutable std::mutex m_lock;
    std::queue<std::shared_ptr<Message>> m_queue;
};
}    // namespace wolkabout

#endif
