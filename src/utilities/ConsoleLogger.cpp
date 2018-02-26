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

#include "utilities/ConsoleLogger.h"
#include <iostream>

namespace wolkabout
{
ConsoleLogger::ConsoleLogger() : m_level{LogLevel::ERROR}
{
}

void ConsoleLogger::logEntry(Log& log)
{
    if (static_cast<int>(log.getLogLevel()) >= static_cast<int>(m_level.load()))
    {
        switch (log.getLogLevel())
        {
        case LogLevel::TRACE:
        {
            std::cout << "[T]" << log.getMessage();
            break;
        }
        case LogLevel::DEBUG:
        {
            std::cout << "[D]" << log.getMessage() << std::endl;
            break;
        }
        case LogLevel::INFO:
        {
            std::cout << "[I]" << log.getMessage();
            break;
        }
        case LogLevel::WARN:
        {
            std::cout << "[W]" << log.getMessage();
            break;
        }
        case LogLevel::ERROR:
        {
            std::cout << "[E]" << log.getMessage();
            break;
        }
        }
    }
}

void ConsoleLogger::setLogLevel(wolkabout::LogLevel level)
{
    m_level = level;
}
}
