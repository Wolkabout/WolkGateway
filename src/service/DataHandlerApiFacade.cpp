#include "DataHandlerApiFacade.h"

namespace wolkabout
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
}    // namespace wolkabout
