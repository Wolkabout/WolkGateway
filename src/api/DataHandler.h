#ifndef WOLKABOUT_DATAHANDLER_H
#define WOLKABOUT_DATAHANDLER_H

#include "core/model/ActuatorStatus.h"
#include "core/model/Alarm.h"
#include "core/model/ConfigurationItem.h"
#include "core/model/DeviceStatus.h"
#include "core/model/SensorReading.h"

#include <string>
#include <vector>

namespace wolkabout
{
class DataHandler
{
public:
    virtual ~DataHandler() = default;

    virtual void addSensorReading(const std::string& deviceKey, const SensorReading& reading) = 0;
    virtual void addSensorReadings(const std::string& deviceKey, const std::vector<SensorReading>& readings) = 0;

    virtual void addAlarm(const std::string& deviceKey, const Alarm& alarm) = 0;

    virtual void addActuatorStatus(const std::string& deviceKey, const ActuatorStatus& status) = 0;

    virtual void addConfiguration(const std::string& deviceKey,
                                  const std::vector<ConfigurationItem>& configurations) = 0;

    virtual void addDeviceStatus(const DeviceStatus& status) = 0;
};
}    // namespace wolkabout

#endif    // WOLKABOUT_DATAHANDLER_H
