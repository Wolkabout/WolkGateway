#include "GatewayFilesystemPersistence.h"

#include "MessagePersister.h"
#include "utilities/FileSystemUtils.h"
#include "utilities/Logger.h"

#include <regex>

namespace
{
const std::string READING_FILE_NAME = "reading_";
const std::string REGEX = READING_FILE_NAME + "(\\d+)";
}    // namespace

namespace wolkabout
{
GatewayFilesystemPersistence::GatewayFilesystemPersistence(const std::string& persistPath)
: m_persister(new MessagePersister()), m_persistPath(persistPath), m_messageNum(0)
{
    initialize();
}

GatewayFilesystemPersistence::~GatewayFilesystemPersistence() = default;

bool GatewayFilesystemPersistence::push(std::shared_ptr<Message> message)
{
    const std::string fileName = READING_FILE_NAME + std::to_string(++m_messageNum);
    const std::string path = m_persistPath + "/" + fileName;
    LOG(INFO) << "Persisting reading " << fileName;

    const auto messageContent = m_persister->save(*message);

    if (!FileSystemUtils::createFileWithContent(path, messageContent))
    {
        LOG(ERROR) << "Failed to persist reading " << fileName;
        return false;
    }

    saveReading(fileName);

    return true;
}

void GatewayFilesystemPersistence::pop()
{
    if (empty())
    {
        return;
    }

    deleteFirstReading();
}

std::shared_ptr<Message> GatewayFilesystemPersistence::front()
{
    if (empty())
    {
        LOG(DEBUG) << "No readings to load";
        return nullptr;
    }

    const auto reading = firstReading();
    const std::string path = readingPath(reading);
    LOG(INFO) << "Loading reading " << reading;

    std::string messageContent;

    if (!FileSystemUtils::readFileContent(path, messageContent))
    {
        LOG(ERROR) << "Failed to read readings file " << reading;

        deleteFirstReading();

        return nullptr;
    }

    return m_persister->load(messageContent);
}

bool GatewayFilesystemPersistence::empty() const
{
    std::lock_guard<std::mutex> guard{m_mutex};
    return m_readingFiles.empty();
}

void GatewayFilesystemPersistence::initialize()
{
    if (FileSystemUtils::isDirectoryPresent(m_persistPath))
    {
        // read file names
        auto files = FileSystemUtils::listFiles(m_persistPath);

        // filter those that match regex
        auto itend = std::remove_if(files.begin(), files.end(),
                                    [](const std::string s) { return !std::regex_match(s, std::regex(REGEX)); });
        files.erase(itend, files.end());

        if (files.size() > 0)
        {
            LOG(INFO) << "WolkPersister: Unpersisting " << static_cast<unsigned int>(files.size()) << " readings";

            // sort filenames by message number
            std::sort(files.begin(), files.end(), [this](const std::string& s1, const std::string& s2) {
                unsigned long fileNumber1, fileNumber2;

                if (!matchFileNumber(s1, fileNumber1))
                {
                    return false;
                }
                else if (!matchFileNumber(s2, fileNumber2))
                {
                    return true;
                }

                return fileNumber1 < fileNumber2;
            });

            // get the highest number
            unsigned long fileNum;
            if (matchFileNumber(files.back(), fileNum))
            {
                m_messageNum = fileNum;
            }

            std::copy(files.begin(), files.end(), std::back_inserter(m_readingFiles));
        }
    }
    else
    {
        if (!FileSystemUtils::createDirectory(m_persistPath))
        {
            LOG(ERROR) << "Could not create presist directory: " << m_persistPath;
        }
    }
}

void GatewayFilesystemPersistence::saveReading(const std::string& fileName)
{
    std::lock_guard<std::mutex> guard{m_mutex};
    m_readingFiles.push_back(fileName);
}

std::string GatewayFilesystemPersistence::readingPath(const std::string& readingFileName) const
{
    return m_persistPath + "/" + readingFileName;
}

void GatewayFilesystemPersistence::deleteFirstReading()
{
    std::lock_guard<std::mutex> guard{m_mutex};

    const std::string path = readingPath(m_readingFiles.front());

    LOG(INFO) << "Deleting reading " << m_readingFiles.front();
    if (FileSystemUtils::deleteFile(path))
    {
        m_readingFiles.pop_front();

        if (m_readingFiles.size() == 0)
        {
            m_messageNum = 0;
        }
    }
    else
    {
        LOG(ERROR) << "Failed to delete readings file " << m_readingFiles.front();
    }
}

std::string GatewayFilesystemPersistence::firstReading()
{
    std::lock_guard<std::mutex> guard{m_mutex};
    return m_readingFiles.front();
}

bool GatewayFilesystemPersistence::matchFileNumber(const std::string& fileName, unsigned long& number) const
{
    const auto fileNameLen = READING_FILE_NAME.length();

    try
    {
        const std::string num = fileName.substr(fileNameLen, fileName.length() - fileNameLen);
        number = std::stoul(num);
        return true;
    }
    catch (...)
    {
        LOG(ERROR) << "Invalid reading file name: " << fileName;
    }

    return false;
}
}    // namespace wolkabout
