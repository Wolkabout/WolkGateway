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

#include "OutboundMessage.h"

#include <string>
#include <utility>

namespace wolkabout
{
OutboundMessage::OutboundMessage(std::string content, std::string topic, unsigned long long itemsCount)
: m_content(std::move(content)), m_topic(std::move(topic)), m_itemsCount(itemsCount)
{
}

const std::string& OutboundMessage::getContent() const
{
    return m_content;
}

const std::string& OutboundMessage::getTopic() const
{
    return m_topic;
}

unsigned long long OutboundMessage::getItemsCount() const
{
    return m_itemsCount;
}
}
