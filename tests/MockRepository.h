#ifndef MOCKREPOSITORY_H
#define MOCKREPOSITORY_H

#include "model/DetailedDevice.h"
#include "repository/DeviceRepository.h"
#include "repository/ExistingDevicesRepository.h"
#include "repository/FileRepository.h"
#include <gmock/gmock.h>

class MockRepository : public wolkabout::DeviceRepository
{
public:
    MockRepository() {}
    virtual ~MockRepository() {}

    void save(const wolkabout::DetailedDevice& /* device */) override{};

    void remove(const std::string& /* devicekey */) override{};

    void removeAll() override{};

    std::unique_ptr<wolkabout::DetailedDevice> findByDeviceKey(const std::string& key) override
    {
        return std::unique_ptr<wolkabout::DetailedDevice>(findByDeviceKeyProxy(key));
    }

    std::unique_ptr<std::vector<std::string>> findAllDeviceKeys() override
    {
        return std::unique_ptr<std::vector<std::string>>(findAllDeviceKeysProxy());
    }

    MOCK_METHOD0(findAllDeviceKeysProxy, std::vector<std::string>*());

    MOCK_METHOD1(containsDeviceWithKey, bool(const std::string& deviceKey));

    MOCK_METHOD1(findByDeviceKeyProxy, wolkabout::DetailedDevice*(const std::string& key));

private:
    GTEST_DISALLOW_COPY_AND_ASSIGN_(MockRepository);
};

class MockFileRepository : public wolkabout::FileRepository
{
public:
    MockFileRepository() {}
    virtual ~MockFileRepository() {}

    std::unique_ptr<wolkabout::FileInfo> getFileInfo(const std::string& fileName) override { return nullptr; };
    std::unique_ptr<std::vector<std::string>> getAllFileNames() override { return nullptr; };

    void store(const wolkabout::FileInfo& info) override{};

    void remove(const std::string& fileName) override{};
    void removeAll() override{};

    bool containsInfoForFile(const std::string& fileName) override { return false; };

private:
    GTEST_DISALLOW_COPY_AND_ASSIGN_(MockFileRepository);
};

class MockExistingDevicesRepository : public wolkabout::ExistingDevicesRepository
{
public:
    MockExistingDevicesRepository() {}
    virtual ~MockExistingDevicesRepository() {}

    void addDeviceKey(const std::string& deviceKey) override {}
    MOCK_METHOD0(getDeviceKeys, std::vector<std::string>());

private:
    GTEST_DISALLOW_COPY_AND_ASSIGN_(MockExistingDevicesRepository);
};

#endif    // MOCKREPOSITORY_H
