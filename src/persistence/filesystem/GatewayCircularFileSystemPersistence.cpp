#include "GatewayCircularFileSystemPersistence.h"
#include "core/model/Message.h"
#include "core/utilities/FileSystemUtils.h"
#include "core/utilities/Logger.h"

namespace wolkabout
{
GatewayCircularFileSystemPersistence::GatewayCircularFileSystemPersistence(const std::string& persistPath,
                                                                           PersistenceMethod method,
                                                                           unsigned sizeLimitBytes)
: GatewayFilesystemPersistence(persistPath, method), m_sizeLimitBytes{sizeLimitBytes}
{
    loadFileSize();
}

bool GatewayCircularFileSystemPersistence::push(std::shared_ptr<wolkabout::Message> message)
{
    std::lock_guard<decltype(m_mutex)> guard{m_mutex};

    auto file = saveToDisk(message);

    if (file.empty())
        return false;

    auto size = static_cast<unsigned>(FileSystemUtils::getFileSize(m_readingFiles.back()));
    m_totalFileSize += size;

    checkLimits();

    return true;
}

void GatewayCircularFileSystemPersistence::pop()
{
    std::lock_guard<decltype(m_mutex)> guard{m_mutex};

    if (empty())
    {
        return;
    }

    const auto reading = m_method == PersistenceMethod::FIFO ? firstReading() : lastReading();

    auto size = static_cast<unsigned>(FileSystemUtils::getFileSize(reading));

    m_totalFileSize = size > m_totalFileSize ? 0 : m_totalFileSize - size;

    m_method == PersistenceMethod::FIFO ? deleteFirstReading() : deleteLastReading();
}

void GatewayCircularFileSystemPersistence::setSizeLimit(unsigned bytes)
{
    LOG(INFO) << "Circular Persistence: Setting size limit " << bytes;

    std::lock_guard<decltype(m_mutex)> guard{m_mutex};
    m_sizeLimitBytes = bytes;

    checkLimits();
}

void GatewayCircularFileSystemPersistence::loadFileSize()
{
    for (const auto& reading : m_readingFiles)
    {
        auto size = static_cast<unsigned>(FileSystemUtils::getFileSize(reading));
        m_totalFileSize += size;
    }
}

void GatewayCircularFileSystemPersistence::checkLimits()
{
    if (m_sizeLimitBytes == 0)
        return;

    while (m_totalFileSize > m_sizeLimitBytes && !m_readingFiles.empty())
    {
        LOG(INFO) << "Circular Persistence: Size over limit " << m_totalFileSize;

        m_method == PersistenceMethod::FIFO ? deleteLastReading() : deleteFirstReading();

        loadFileSize();
    }
}
}    // namespace wolkabout
