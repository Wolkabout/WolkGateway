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

#ifndef BUFFER_H
#define BUFFER_H

#include <condition_variable>
#include <mutex>
#include <queue>

namespace wolkabout
{
template <class T> class Buffer
{
public:
    Buffer() = default;
    virtual ~Buffer() = default;

    void push(T item);
    void push_rvalue(T&& item);

    T pop();

    bool isEmpty() const;

    void swapBuffers();

private:
    std::queue<T> m_pushQueue;
    std::queue<T> m_popQueue;

    mutable std::mutex m_lock;
    std::condition_variable m_condition;
};

template <class T> void Buffer<T>::push(T item)
{
    std::unique_lock<std::mutex> unique_lock(m_lock);

    m_pushQueue.push(std::move(item));

    m_condition.notify_one();
}

template <class T> void Buffer<T>::push_rvalue(T&& item)
{
    std::unique_lock<std::mutex> unique_lock(m_lock);

    m_pushQueue.push(std::move(item));

    m_condition.notify_one();
}

template <class T> T Buffer<T>::pop()
{
    if (m_popQueue.empty())
    {
        return nullptr;
    }

    T item = std::move(m_popQueue.front());
    m_popQueue.pop();
    return item;
}

template <class T> void Buffer<T>::swapBuffers()
{
    std::unique_lock<std::mutex> unique_lock(m_lock);

    if (m_pushQueue.empty())
    {
        m_condition.wait(unique_lock);
    }

    std::swap(m_pushQueue, m_popQueue);
}

template <class T> bool Buffer<T>::isEmpty() const
{
    std::unique_lock<std::mutex> unique_lock(m_lock);

    return m_pushQueue.empty();
}
}    // namespace wolkabout

#endif
