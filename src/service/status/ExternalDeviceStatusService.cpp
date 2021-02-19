#include "ExternalDeviceStatusService.h"

#include "utilities/Logger.h"

namespace wolkabout
{
void ExternalDeviceStatusService::connected() {}

void ExternalDeviceStatusService::disconnected() {}

void ExternalDeviceStatusService::addDeviceStatus(const DeviceStatus& status)
{
    sendStatusUpdateForDevice(status.getDeviceKey(), status.getStatus());
}

void ExternalDeviceStatusService::requestDeviceStatus(const std::string& /*deviceKey*/)
{
    LOG(WARN) << "Not handling device status request";
}

}    // namespace wolkabout
