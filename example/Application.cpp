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

#include "Wolk.h"
#include "service/FirmwareInstaller.h"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <string>

#include "utilities/ConsoleLogger.h"

int main(int /* argc */, char** /* argv */)
{
    auto logger = std::unique_ptr<wolkabout::ConsoleLogger>(new wolkabout::ConsoleLogger());
    logger->setLogLevel(wolkabout::LogLevel::INFO);
    wolkabout::Logger::setInstance(std::move(logger));

    wolkabout::DeviceManifest deviceManifest;
    wolkabout::Device device("device_name", "device_key", "device_password", deviceManifest);

    std::unique_ptr<wolkabout::Wolk> wolk =
      wolkabout::Wolk::newBuilder(device)
        .build();

    wolk->connect();

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
