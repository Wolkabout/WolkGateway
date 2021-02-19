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

    void addSensorReading(const std::string& deviceKey, const SensorReading& reading) override;
    void addSensorReadings(const std::string& deviceKey, const std::vector<SensorReading>& readings) override;
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
