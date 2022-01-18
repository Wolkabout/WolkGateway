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

#include "gateway/persistence/filesystem/MessagePersister.h"

#include <sstream>

namespace
{
const std::string DELIMITER = "\n";
}

namespace wolkabout
{
namespace gateway
{
std::string MessagePersister::save(const wolkabout::Message& message) const
{
    std::stringstream stream;

    stream << message.getChannel() << DELIMITER << message.getContent();

    return stream.str();
}

std::unique_ptr<Message> MessagePersister::load(const std::string& text) const
{
    const auto pos = text.find(DELIMITER);

    if (pos == std::string::npos)
    {
        return nullptr;
    }

    const auto channel = text.substr(0, pos);
    const auto content = text.substr(pos + DELIMITER.size());

    return std::unique_ptr<Message>(new Message(channel, content));
}
}    // namespace gateway
}    // namespace wolkabout
