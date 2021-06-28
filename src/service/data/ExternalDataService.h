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

#ifndef WOLKABOUT_EXTERNALDATASERVICE_H
#define WOLKABOUT_EXTERNALDATASERVICE_H

#include "DataService.h"
#include "api/DataHandler.h"

namespace wolkabout
{
class ExternalDataService : public DataService
{
public:
    using DataService::DataService;

    void addSensorReading(const std::string& deviceKey, const SensorReading& reading);
    void addSensorReadings(const std::string& deviceKey, const std::vector<SensorReading>& readings);
    void addAlarm(const std::string& deviceKey, const Alarm& alarm);
    void addActuatorStatus(const std::string& deviceKey, const ActuatorStatus& status);
    void addConfiguration(const std::string& deviceKey, const std::vector<ConfigurationItem>& configurations);

    void requestActuatorStatusesForDevice(const std::string& deviceKey) override;
    void requestActuatorStatusesForAllDevices() override;

private:
    void handleMessageForDevice(std::shared_ptr<Message> message) override;
};
}    // namespace wolkabout

#endif    // WOLKABOUT_EXTERNALDATASERVICE_H
