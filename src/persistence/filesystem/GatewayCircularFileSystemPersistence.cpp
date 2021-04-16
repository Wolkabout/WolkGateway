#include "GatewayCircularFileSystemPersistence.h"
#include "core/utilities/FileSystemUtils.h"
#include "core/utilities/Logger.h"

wolkabout::GatewayCircularFileSystemPersistence::GatewayCircularFileSystemPersistence(const std::string& persistPath,
                                                                                      unsigned sizeLimitBytes)
: GatewayFilesystemPersistence(persistPath), m_sizeLimitBytes{sizeLimitBytes}
{
    loadFileSize();
}

bool wolkabout::GatewayCircularFileSystemPersistence::push(std::shared_ptr<wolkabout::Message> message)
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

void wolkabout::GatewayCircularFileSystemPersistence::pop()
{
    std::lock_guard<decltype(m_mutex)> guard{m_mutex};

    if (empty())
    {
        return;
    }

    auto size = static_cast<unsigned>(FileSystemUtils::getFileSize(m_readingFiles.back()));

    m_totalFileSize = size > m_totalFileSize ? 0 : m_totalFileSize - size;

    deleteLastReading();
}

void wolkabout::GatewayCircularFileSystemPersistence::setSizeLimit(unsigned bytes)
{
    LOG(INFO) << "Circular Persistence: Setting size limit " << bytes;

    std::lock_guard<decltype(m_mutex)> guard{m_mutex};
    m_sizeLimitBytes = bytes;

    checkLimits();
}

void wolkabout::GatewayCircularFileSystemPersistence::loadFileSize()
{
    for (const auto& reading : m_readingFiles)
    {
        auto size = static_cast<unsigned>(FileSystemUtils::getFileSize(reading));
        m_totalFileSize += size;
    }
}

void wolkabout::GatewayCircularFileSystemPersistence::checkLimits()
{
    if (m_sizeLimitBytes == 0)
        return;

    while (m_totalFileSize > m_sizeLimitBytes && !m_readingFiles.empty())
    {
        LOG(INFO) << "Circular Persistence: Size over limit " << m_totalFileSize;
        deleteFirstReading();
    }
}
