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

#include "DataHandlerApiFacade.h"

namespace wolkabout
{
namespace gateway
{
DataHandlerApiFacade::DataHandlerApiFacade(ExternalDataService& dataHandler, ExternalDeviceStatusService& statusHandler)
: m_dataHandler{dataHandler}, m_statusHandler{statusHandler}
{
}

void DataHandlerApiFacade::addSensorReading(const std::string& deviceKey, const SensorReading& reading)
{
    m_dataHandler.addSensorReading(deviceKey, reading);
}

void DataHandlerApiFacade::addSensorReadings(const std::string& deviceKey, const std::vector<SensorReading>& readings)
{
    m_dataHandler.addSensorReadings(deviceKey, readings);
}

void DataHandlerApiFacade::addAlarm(const std::string& deviceKey, const Alarm& alarm)
{
    m_dataHandler.addAlarm(deviceKey, alarm);
}

void DataHandlerApiFacade::addActuatorStatus(const std::string& deviceKey, const ActuatorStatus& status)
{
    m_dataHandler.addActuatorStatus(deviceKey, status);
}

void DataHandlerApiFacade::addConfiguration(const std::string& deviceKey,
                                            const std::vector<ConfigurationItem>& configurations)
{
    m_dataHandler.addConfiguration(deviceKey, configurations);
}

void DataHandlerApiFacade::addDeviceStatus(const DeviceStatus& status)
{
    m_statusHandler.addDeviceStatus(status);
}
}    // namespace gateway
}    // namespace wolkabout
