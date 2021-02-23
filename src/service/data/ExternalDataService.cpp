#include "ExternalDataService.h"

#include "model/Message.h"
#include "protocol/DataProtocol.h"
#include "utilities/Logger.h"

#include <algorithm>

namespace wolkabout
{
void ExternalDataService::addSensorReading(const std::string& deviceKey, const SensorReading& reading)
{
    const std::shared_ptr<Message> message =
      m_protocol.makeMessage(deviceKey, {std::make_shared<SensorReading>(reading)});

    routeDeviceToPlatformMessage(message);
}

void ExternalDataService::addSensorReadings(const std::string& deviceKey, const std::vector<SensorReading>& readings)
{
    if (readings.empty())
    {
        return;
    }

    std::vector<std::shared_ptr<SensorReading>> parsableReadings;

    std::transform(readings.begin(), readings.end(), std::back_inserter(parsableReadings),
                   [](const SensorReading& reading) { return std::make_shared<SensorReading>(reading); });

    const std::shared_ptr<Message> message = m_protocol.makeMessage(deviceKey, parsableReadings);

    routeDeviceToPlatformMessage(message);
}

void ExternalDataService::addAlarm(const std::string& deviceKey, const Alarm& alarm)
{
    const std::shared_ptr<Message> message = m_protocol.makeMessage(deviceKey, {std::make_shared<Alarm>(alarm)});

    routeDeviceToPlatformMessage(message);
}

void ExternalDataService::addActuatorStatus(const std::string& deviceKey, const ActuatorStatus& status)
{
    const std::shared_ptr<Message> message =
      m_protocol.makeMessage(deviceKey, {std::make_shared<ActuatorStatus>(status)});

    routeDeviceToPlatformMessage(message);
}

void ExternalDataService::addConfiguration(const std::string& deviceKey,
                                           const std::vector<ConfigurationItem>& configurations)
{
    const std::shared_ptr<Message> message = m_protocol.makeMessage(deviceKey, configurations);

    routeDeviceToPlatformMessage(message);
}

void ExternalDataService::requestActuatorStatusesForDevice(const std::string& /*deviceKey*/)
{
    LOG(WARN) << "Not requesting actuator status for device";
}

void ExternalDataService::requestActuatorStatusesForAllDevices()
{
    LOG(WARN) << "Not handling message for devices";
}

void ExternalDataService::handleMessageForDevice(std::shared_ptr<Message> /*message*/)
{
    LOG(WARN) << "Not handling message for device";
}

}    // namespace wolkabout
