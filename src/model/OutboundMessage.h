/*
 * Copyright 2017 WolkAbout Technology s.r.o.
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

#ifndef OUTBOUNDMESSAGE_H
#define OUTBOUNDMESSAGE_H

#include <string>

namespace wolkabout
{
class OutboundMessage
{
public:
    OutboundMessage(std::string content, std::string topic, unsigned long long int itemsCount);
    virtual ~OutboundMessage() = default;

    const std::string& getContent() const;
    const std::string& getTopic() const;
    unsigned long long getItemsCount() const;

private:
    std::string m_content;
    std::string m_topic;
    unsigned long long int m_itemsCount;
};
}

#endif
