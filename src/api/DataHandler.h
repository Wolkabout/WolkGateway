#ifndef WOLKABOUT_DATAHANDLER_H
#define WOLKABOUT_DATAHANDLER_H

#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/ConfigurationItem.h"
#include "model/DeviceStatus.h"
#include "model/SensorReading.h"

#include <string>
#include <vector>

namespace wolkabout
{
class DataHandler
{
public:
    virtual ~DataHandler() = default;

    virtual void addSensorReading(const std::string& deviceKey, const SensorReading& reading) = 0;

    virtual void addAlarm(const std::string& deviceKey, const Alarm& alarm) = 0;

    virtual void addActuatorStatus(const std::string& deviceKey, const ActuatorStatus& status) = 0;

    virtual void addConfiguration(const std::string& deviceKey,
                                  const std::vector<ConfigurationItem>& configurations) = 0;

    virtual void addDeviceStatus(const DeviceStatus& status) = 0;
};
}    // namespace wolkabout

#endif    // WOLKABOUT_DATAHANDLER_H
