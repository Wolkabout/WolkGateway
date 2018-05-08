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

#ifndef GATEWAYPERSISTENCE_H
#define GATEWAYPERSISTENCE_H

#include <memory>

namespace wolkabout
{
class Message;

/**
 * @brief A storage designed for holding messages in persistent store prior to publishing.
 *
 * Implementation storing/retrieving strategy must be FIFO.
 *
 * All methods must be implemented in a thread safe manner.
 */
class GatewayPersistence
{
public:
    /**
     * @brief Destructor
     */
    virtual ~GatewayPersistence() = default;

    /**
     * @brief Inserts the wolkabout::Message
     *
     * @param message to be inserted
     * @return {@code true} if successful, or {@code false} if
     * element can not be inserted
     */
    virtual bool push(std::shared_ptr<Message> message) = 0;

    /**
     * @brief Retrieves, first wolkabout::Message of this storage and removes it from storage.
     *
     * @return Message {@code std::shared_ptr<Message>} or returns nullptr {@code std::shared_ptr<Message>} if this
     * storage is empty.
     */
    virtual std::shared_ptr<Message> pop() = 0;

    /**
     * @brief Retrieves, first wolkabout::Message of this storage without removing it from storage.
     *
     * @return Message {@code std::shared_ptr<Message>} or returns nullptr {@code std::shared_ptr<Message>} if this
     * storage is empty.
     */
    virtual std::shared_ptr<Message> front() = 0;

    /**
     * Returns whether this storage contains any messages.
     *
     * @return {@code true} if this storage contains no wolkabout::Message
     */
    virtual bool empty() const = 0;
};
}    // namespace wolkabout

#endif
