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

#include "Wolk.h"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <string>

int main(int /* argc */, char** /* argv */)
{
    wolkabout::Device device("device_key", "some_password", {"SW", "SL"});

    static bool switchValue = false;
    static int sliderValue = 0;

    std::unique_ptr<wolkabout::Wolk> wolk =
      wolkabout::Wolk::newBuilder(device)
            .actuationHandler([&](const std::string& reference, const std::string& value) -> void {
        std::cout << "Actuation request received - Reference: " << reference << " value: " << value << std::endl;

        if (reference == "SW") {
            switchValue = value == "true" ? true : false;
        }
        else if (reference == "SL") {
            try {
                sliderValue = std::stoi(value);
            } catch (...) {
                sliderValue = 0;
            }
        }
    })
            .actuatorStatusProvider([&](const std::string& reference) -> wolkabout::ActuatorStatus {
        if (reference == "SW") {
            return wolkabout::ActuatorStatus(switchValue ? "true" : "false", wolkabout::ActuatorStatus::State::READY);
        } else if (reference == "SL") {
            return wolkabout::ActuatorStatus(std::to_string(sliderValue), wolkabout::ActuatorStatus::State::READY);
        }

        return wolkabout::ActuatorStatus("", wolkabout::ActuatorStatus::State::READY);
        })
        .build();

    wolk->connect();

    wolk->addSensorReading("P", 1024);
    wolk->addSensorReading("T", 25.6);
    wolk->addSensorReading("H", 52);

    wolk->addAlarm("HH", "High Humidity");

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
