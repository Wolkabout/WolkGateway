#ifndef WOLKABOUT_EXTERNALDATASERVICE_H
#define WOLKABOUT_EXTERNALDATASERVICE_H

#include "DataService.h"
#include "api/DataHandler.h"

namespace wolkabout
{
class ExternalDataService : public DataService, public DataHandler
{
public:
    using DataService::DataService;

    void addSensorReading(const std::string& deviceKey, const SensorReading& reading) override;
    void addSensorReadings(const std::string& deviceKey, const std::vector<SensorReading>& readings) override;
    void addAlarm(const std::string& deviceKey, const Alarm& alarm) override;
    void addActuatorStatus(const std::string& deviceKey, const ActuatorStatus& status) override;
    void addConfiguration(const std::string& deviceKey, const std::vector<ConfigurationItem>& configurations) override;
    void addDeviceStatus(const DeviceStatus& status) override;

    void requestActuatorStatusesForDevice(const std::string& deviceKey) override;
    void requestActuatorStatusesForAllDevices() override;

private:
    void handleMessageForDevice(std::shared_ptr<Message> message) override;
};
}    // namespace wolkabout

#endif    // WOLKABOUT_EXTERNALDATASERVICE_H
