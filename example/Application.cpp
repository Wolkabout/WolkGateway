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

#include "ActuatorStatus.h"
#include "Device.h"
#include "Wolk.h"

#include <iostream>
#include <memory>
#include <chrono>
#include <thread>
#include <string>

int main(int /* argc */, char** /* argv */)
{
    wolkabout::Device device;
    device.setDeviceKey("DEVICE_KEY")
      .setDevicePassword("DEVICE_PASSWORD")
      .setActuatorReferences({"ACTUATOR_REFERENCE_ONE", "ACTUATOR_REFERENCE_TWO", "ACTUATOR_REFERENCE_THREE"});

    std::unique_ptr<wolkabout::Wolk> wolk =
      wolkabout::Wolk::connectDevice(device)
        .actuationHandler([](const std::string& reference, const std::string& value) -> void {
            std::cout << "Actuation request received - Reference: " << reference << " value: " << value << std::endl;
        })

        .actuatorStatusProvider([](const std::string& reference) -> wolkabout::ActuatorStatus {
            if (reference == "ACTUATOR_REFERENCE_ONE") {
                return wolkabout::ActuatorStatus("65", wolkabout::ActuatorStatus::State::READY);
            } else if (reference == "ACTUATOR_REFERENCE_TWO") {
                return wolkabout::ActuatorStatus("false", wolkabout::ActuatorStatus::State::READY);;
            }

            return wolkabout::ActuatorStatus("", wolkabout::ActuatorStatus::State::READY);
        })
        .connect();

    wolk->addSensorReading("TEMPERATURE_REF", 23.4);
    wolk->addSensorReading("BOOL_SENSOR_REF", true);

    wolk->addAlarm("ALARM_REF", "ALARM_MESSAGE_FROM_CONNECTOR");

    while(true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    return 0;
}
