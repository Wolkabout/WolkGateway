#ifndef DEVICEREPOSITORY_H
#define DEVICEREPOSITORY_H

#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class Device;
class DeviceRepository
{
public:
    virtual ~DeviceRepository() = default;

    virtual void save(std::shared_ptr<Device> device) = 0;

    virtual void update(std::shared_ptr<Device> device) = 0;

    virtual void remove(const std::string& devicekey) = 0;

    virtual std::shared_ptr<Device> findByDeviceKey(const std::string& key) = 0;

    virtual std::shared_ptr<std::vector<std::string>> findAllDeviceKeys() = 0;
};
}    // namespace wolkabout

#endif    // DEVICEREPOSITORY_H
