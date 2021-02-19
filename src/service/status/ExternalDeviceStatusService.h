#ifndef EXTERNALDEVICESTATUSSERVICE_H
#define EXTERNALDEVICESTATUSSERVICE_H

#include "DeviceStatusService.h"

namespace wolkabout
{
class ExternalDeviceStatusService : public DeviceStatusService
{
public:
    using DeviceStatusService::DeviceStatusService;

    void connected() override;
    void disconnected() override;

    void addDeviceStatus(const DeviceStatus& status);

private:
    void requestDeviceStatus(const std::string& deviceKey) override;
};
}    // namespace wolkabout

#endif    // EXTERNALDEVICESTATUSSERVICE_H
