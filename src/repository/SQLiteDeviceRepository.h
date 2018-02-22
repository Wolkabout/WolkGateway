#ifndef DEVICEREPOSITORYIMPL_H
#define DEVICEREPOSITORYIMPL_H

#include "Poco/Data/Session.h"
#include "repository/DeviceRepository.h"

#include <memory>
#include <mutex>
#include <string>

namespace wolkabout
{
class SQLiteDeviceRepository : public DeviceRepository
{
public:
    SQLiteDeviceRepository(const std::string& connectionString = "deviceRepository.db");
    virtual ~SQLiteDeviceRepository() = default;

    virtual void save(std::shared_ptr<Device> device) override;

    virtual void update(std::shared_ptr<Device> device) override;

    virtual void remove(const std::string& deviceKey) override;

    virtual std::shared_ptr<Device> findByDeviceKey(const std::string& deviceKey) override;

    virtual std::shared_ptr<std::vector<std::string>> findAllDeviceKeys() override;

private:
    std::mutex m_mutex;
    std::unique_ptr<Poco::Data::Session> m_session;
};
}    // namespace wolkabout

#endif    // DEVICEREPOSITORYIMPL_H
