#include "ExternalDataService.h"

#include "model/Message.h"
#include "protocol/DataProtocol.h"
#include "utilities/Logger.h"

namespace wolkabout
{
void ExternalDataService::addSensorReading(const std::string& deviceKey, const SensorReading& reading)
{
    const std::shared_ptr<Message> message =
      m_protocol.makeMessage(deviceKey, {std::make_shared<SensorReading>(reading)});

    addMessage(message);
}

void ExternalDataService::addAlarm(const std::string& deviceKey, const Alarm& alarm)
{
    const std::shared_ptr<Message> message = m_protocol.makeMessage(deviceKey, {std::make_shared<Alarm>(alarm)});

    addMessage(message);
}

void ExternalDataService::addActuatorStatus(const std::string& deviceKey, const ActuatorStatus& status)
{
    const std::shared_ptr<Message> message =
      m_protocol.makeMessage(deviceKey, {std::make_shared<ActuatorStatus>(status)});

    addMessage(message);
}

void ExternalDataService::addConfiguration(const std::string& deviceKey,
                                           const std::vector<ConfigurationItem>& configurations)
{
    const std::shared_ptr<Message> message = m_protocol.makeMessage(deviceKey, configurations);

    addMessage(message);
}

void ExternalDataService::addDeviceStatus(const DeviceStatus& status)
{
    // TODO
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
