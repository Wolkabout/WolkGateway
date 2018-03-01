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

#include "Logger.h"
#include <algorithm>
#include <iostream>

namespace wolkabout
{
std::unique_ptr<Logger> Logger::m_instance;

void Logger::operator+=(Log& log)
{
    logEntry(log);
}

void Logger::setInstance(std::unique_ptr<Logger> instance)
{
    m_instance = std::move(instance);
}

Logger* Logger::getInstance()
{
    return m_instance.get();
}

Log::Log(LogLevel level) : m_level{level}, m_message{""} {}

LogLevel Log::getLogLevel() const
{
    return m_level;
}

std::string Log::getMessage() const
{
    return m_message.str();
}

wolkabout::LogLevel from_string(std::string level)
{
    std::transform(level.begin(), level.end(), level.begin(), ::toupper);

    if (level.compare("TRACE") == 0)
    {
        return wolkabout::LogLevel::TRACE;
    }
    else if (level.compare("DEBUG") == 0)
    {
        return wolkabout::LogLevel::DEBUG;
    }
    else if (level.compare("INFO") == 0)
    {
        return wolkabout::LogLevel::INFO;
    }
    else if (level.compare("WARN") == 0)
    {
        return wolkabout::LogLevel::WARN;
    }
    else
    {
        return wolkabout::LogLevel::ERROR;
    }
}
}    // namespace wolkabout
