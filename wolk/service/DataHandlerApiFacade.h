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

#ifndef DATAHANDLERAPIFACADE_H
#define DATAHANDLERAPIFACADE_H

#include "api/DataHandler.h"
#include "data/ExternalDataService.h"
#include "status/ExternalDeviceStatusService.h"

namespace wolkabout
{
class DataHandlerApiFacade : public DataHandler
{
public:
    DataHandlerApiFacade(ExternalDataService& dataHandler, ExternalDeviceStatusService& statusHandler);

    void addReading(const std::string& deviceKey, const SensorReading& reading) override;
    void addReadings(const std::string& deviceKey, const std::vector<SensorReading>& readings) override;
    void addAlarm(const std::string& deviceKey, const Alarm& alarm) override;
    void addActuatorStatus(const std::string& deviceKey, const ActuatorStatus& status) override;
    void addConfiguration(const std::string& deviceKey, const std::vector<ConfigurationItem>& configurations) override;
    void addDeviceStatus(const DeviceStatus& status) override;

private:
    ExternalDataService& m_dataHandler;
    ExternalDeviceStatusService& m_statusHandler;
};
}    // namespace wolkabout

#endif    // DATAHANDLERAPIFACADE_H
