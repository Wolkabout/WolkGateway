#ifndef DEVICEREGISTRATIONREQUESTDTO_H
#define DEVICEREGISTRATIONREQUESTDTO_H

#include "model/Device.h"
#include "model/DeviceManifest.h"

#include <string>

namespace wolkabout
{
class DeviceRegistrationRequestDto
{
public:
    DeviceRegistrationRequestDto() = default;
    DeviceRegistrationRequestDto(std::string deviceName, std::string deviceKey, DeviceManifest deviceManifest);
    DeviceRegistrationRequestDto(Device device);

    virtual ~DeviceRegistrationRequestDto() = default;

    const std::string& getDeviceName() const;
    const std::string& getDeviceKey() const;

    const DeviceManifest& getManifest() const;

private:
    Device m_device;
};
}    // namespace wolkabout

#endif    // DEVICEREGISTRATIONDTO_H
