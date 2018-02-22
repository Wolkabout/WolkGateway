#include "Device.h"

#include <string>
#include <utility>

namespace wolkabout
{
Device::Device(std::string name, std::string key, DeviceManifest deviceManifest)
: Device(std::move(name), std::move(key), "", std::move(deviceManifest))
{
}

Device::Device(std::string name, std::string key, std::string password, DeviceManifest deviceManifest)
: m_name(std::move(name))
, m_key(std::move(key))
, m_password(std::move(password))
, m_deviceManifest(std::move(deviceManifest))
{
}

const std::string& Device::getName() const
{
    return m_name;
}

const std::string& Device::getKey() const
{
    return m_key;
}

const std::string& Device::getPassword() const
{
    return m_password;
}

const DeviceManifest& Device::getManifest() const
{
    return m_deviceManifest;
}

std::vector<std::string> Device::getActuatorReferences() const
{
    std::vector<std::string> actuatorReferences(m_deviceManifest.getActuators().size());
    for (const ActuatorManifest& actuatorManifest : m_deviceManifest.getActuators())
    {
        actuatorReferences.push_back(actuatorManifest.getReference());
    }

    return actuatorReferences;
}
}    // namespace wolkabout
