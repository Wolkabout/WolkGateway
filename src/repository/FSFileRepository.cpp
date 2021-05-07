#include "FSFileRepository.h"

#include "core/utilities/FileSystemUtils.h"
#include "core/utilities/Logger.h"
#include "core/utilities/StringUtils.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <utility>

namespace wolkabout
{
const std::string FILE_SYSTEM_DIVIDER = "/";

FSFileRepository::FSFileRepository(std::string folderPath) : m_folderPath(std::move(folderPath))
{
    // Check that the folder exists
    if (!FileSystemUtils::isDirectoryPresent(m_folderPath))
        throw std::runtime_error("FSFileRepository: The folder '" + m_folderPath + "' does not exist!");

    LOG(TRACE) << "FSFileRepository: Created in folder '" << m_folderPath << "'.";
}

std::unique_ptr<FileInfo> FSFileRepository::getFileInfo(const std::string& fileName)
{
    // Check if the file is found in the folder.
    const auto filePath = composeFilePath(fileName);
    if (!FileSystemUtils::isFilePresent(filePath))
    {
        LOG(DEBUG) << "FSFileRepository: Failed to obtain `FileInfo` for a file '" << fileName << "'. File not found.";
        return nullptr;
    }

    // Get the hash value for this file
    auto hash = calculateFileHash(filePath);
    LOG(DEBUG) << "FSFileRepository: Obtained info about file '" << fileName << "', hash: '" << hash << "', path: '"
               << filePath << "'";

    // Return the `FileInfo` about this file.
    return std::unique_ptr<FileInfo>(new FileInfo(fileName, hash, filePath));
}

std::unique_ptr<std::vector<std::string>> FSFileRepository::getAllFileNames()
{
    // Get the list of files in the folder
    auto files = FileSystemUtils::listFiles(m_folderPath);
    LOG(DEBUG) << "FSFileRepository: Obtained " << files.size() << " files.";

    // And return the list of files
    auto fileVector = std::unique_ptr<std::vector<std::string>>(new std::vector<std::string>());
    if (!files.empty())
        fileVector->swap(files);
    return fileVector;
}

void FSFileRepository::store(const FileInfo& /** info */)
{
    // This doesn't have to do anything, as the file should already be in the directory.
}

void FSFileRepository::remove(const std::string& fileName)
{
    // Check if the file exists
    const auto filePath = composeFilePath(fileName);
    if (FileSystemUtils::isFilePresent(filePath))
    {
        FileSystemUtils::deleteFile(filePath);
        LOG(DEBUG) << "FSFileRepository: File '" << fileName << "' has been deleted.";
    }
}

void FSFileRepository::removeAll()
{
    // Go through all the files in the folder and remove them all
    for (const auto& fileName : FileSystemUtils::listFiles(m_folderPath))
    {
        const auto filePath = composeFilePath(fileName);
        FileSystemUtils::deleteFile(filePath);
        LOG(DEBUG) << "FSFileRepository: File '" << fileName << "' has been deleted.";
    }
}

bool FSFileRepository::containsInfoForFile(const std::string& fileName)
{
    // Check if the file exists
    const auto filePath = composeFilePath(fileName);
    return FileSystemUtils::isFilePresent(filePath);
}

std::string FSFileRepository::composeFilePath(const std::string& fileName)
{
    return m_folderPath + FILE_SYSTEM_DIVIDER + fileName;
}

std::string FSFileRepository::calculateFileHash(const std::string& filePath)
{
    LOG(TRACE) << "FSFileRepository: Calculating hash for file at '" << filePath << "'.";

    // Check if the file exists
    if (!FileSystemUtils::isFilePresent(filePath))
    {
        LOG(TRACE) << "FSFileRepository: File could not be found at location '" << filePath << "'.";
        return "";
    }

    // Load the entire content of the file, and get the hash.
    std::string stringContent;
    if (!FileSystemUtils::readFileContent(filePath, stringContent))
    {
        LOG(TRACE) << "FSFileRepository: Failed to read content of file at location '" << filePath << "'.";
        return "";
    }

    // Calculate the hash
    const auto hash = wolkabout::ByteUtils::hashSHA256(wolkabout::ByteUtils::toByteArray(stringContent));

    // Generate the hash string
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto byte : hash)
        ss << std::setw(2) << static_cast<int>(byte);

    // Return the generated string
    return ss.str();
}
}    // namespace wolkabout
