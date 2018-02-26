#ifndef DEVICE_H
#define DEVICE_H

#include "model/DeviceManifest.h"

#include <string>

namespace wolkabout
{
class Device
{
public:
    Device() = default;
    Device(std::string name, std::string key, DeviceManifest deviceManifest);
    Device(std::string name, std::string key, std::string password, DeviceManifest deviceManifest);

    virtual ~Device() = default;

    const std::string& getName() const;
    const std::string& getKey() const;
    const std::string& getPassword() const;

    const DeviceManifest& getManifest() const;

    std::vector<std::string> getActuatorReferences() const;

private:
    std::string m_name;
    std::string m_key;
    std::string m_password;

    DeviceManifest m_deviceManifest;
};
}    // namespace wolkabout

#endif    // DEVICE_H
