#include "model/DeviceRegistrationRequestDto.h"
#include "model/Device.h"
#include "model/DeviceManifest.h"

#include <string>
#include <utility>

namespace wolkabout
{
DeviceRegistrationRequestDto::DeviceRegistrationRequestDto(std::string deviceName, std::string deviceKey,
                                                           DeviceManifest deviceManifest)
: m_device(std::move(deviceName), std::move(deviceKey), std::move(deviceManifest))
{
}

DeviceRegistrationRequestDto::DeviceRegistrationRequestDto(Device device) : m_device(std::move(device))
{
}

const std::string& DeviceRegistrationRequestDto::getDeviceName() const
{
    return m_device.getName();
}

const std::string& DeviceRegistrationRequestDto::getDeviceKey() const
{
    return m_device.getKey();
}

const DeviceManifest& DeviceRegistrationRequestDto::getManifest() const
{
    return m_device.getManifest();
}
}    // namespace wolkabout
