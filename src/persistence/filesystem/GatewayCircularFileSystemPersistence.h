#ifndef GATEWAYCIRCULARFILESYSTEMPERSISTENCE_H
#define GATEWAYCIRCULARFILESYSTEMPERSISTENCE_H

#include "GatewayFilesystemPersistence.h"

namespace wolkabout
{
/**
 * @brief The GatewayCircularFileSystemPersistence class
 * LIFO
 */
class GatewayCircularFileSystemPersistence : public GatewayFilesystemPersistence
{
public:
    explicit GatewayCircularFileSystemPersistence(const std::string& persistPath, unsigned sizeLimitBytes = 0);

    bool push(std::shared_ptr<Message> message) override;
    void pop() override;

    void setSizeLimit(unsigned bytes);

private:
    void loadFileSize();
    void checkLimits();

    unsigned m_sizeLimitBytes;
    unsigned long long m_totalFileSize = 0;
};
}    // namespace wolkabout

#endif    // GATEWAYCIRCULARFILESYSTEMPERSISTENCE_H
