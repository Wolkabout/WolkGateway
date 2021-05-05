#include "FSFileRepository.h"

#include "core/utilities/FileSystemUtils.h"
#include "core/utilities/Logger.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <openssl/sha.h>
#include <utility>

namespace wolkabout
{
const std::string HASH_FILE = ".hash-file";
const std::string FILE_SYSTEM_DIVIDER = "/";

const std::string FILE_NAME_COLUMN = "FileName";
const std::string FILE_HASH_COLUMN = "FileHash";
const std::string FILE_PATH_COLUMN = "FilePath";
const std::string COLUMN_DIVIDER = ",";

FSFileRepository::FSFileRepository(std::string folderPath)
: m_folderPath(std::move(folderPath)), m_hashFilePath(m_folderPath + FILE_SYSTEM_DIVIDER + HASH_FILE)
{
    // Check that the folder exists
    if (!FileSystemUtils::isDirectoryPresent(m_folderPath))
        throw std::runtime_error("FSFileRepository: The folder '" + m_folderPath + "' does not exist!");
}

std::unique_ptr<FileInfo> FSFileRepository::getFileInfo(const std::string& fileName)
{
    // Check if the file is found in the folder.
    const auto filePath = m_folderPath + FILE_SYSTEM_DIVIDER + fileName;
    if (!FileSystemUtils::isFilePresent(filePath))
    {
        LOG(DEBUG) << "FSFileRepository: Failed to obtain `FileInfo` for a file '" << fileName << "'. File not found.";
        return nullptr;
    }

    // Get the hash value for this file
    auto hash = obtainFileHash(fileName);

    // Return the `FileInfo` about this file.
    return std::unique_ptr<FileInfo>(new FileInfo(fileName, hash, filePath));
}

std::unique_ptr<std::vector<std::string>> FSFileRepository::getAllFileNames()
{
    // Get the list of files in the folder
    auto files = FileSystemUtils::listFiles(m_folderPath);

    // Filter out our hash file if it is to be found there
    const auto& it = std::find(files.cbegin(), files.cend(), HASH_FILE);
    if (it != files.cend())
        files.erase(it);

    // And return the list of files
    auto fileVector = std::unique_ptr<std::vector<std::string>>();
    fileVector->swap(files);
    return fileVector;
}

void FSFileRepository::store(const FileInfo& info)
{
    // Append the object into the file (and create if it does not exist)
    createHashFile();
    appendFileInfoIntoHashFile(info);
}

void FSFileRepository::remove(const std::string& fileName)
{
    // Check if the file exists
    const auto filePath = m_folderPath + FILE_SYSTEM_DIVIDER + fileName;
    if (FileSystemUtils::isFilePresent(filePath))
    {
        FileSystemUtils::deleteFile(filePath);
        LOG(DEBUG) << "FSFileRepository: File '" << fileName << "' has been deleted.";
    }
}

void FSFileRepository::removeAll()
{
    // Go through all the files in the folder and remove them all
    for (const auto& file : FileSystemUtils::listFiles(m_folderPath))
    {
        const auto filePath = m_folderPath + FILE_SYSTEM_DIVIDER + std::string(file);
        FileSystemUtils::deleteFile(filePath);
        LOG(DEBUG) << "FSFileRepository: File '" << file << "' has been deleted.";
    }
}

bool FSFileRepository::containsInfoForFile(const std::string& fileName)
{
    // Check if the file exists
    const auto filePath = m_folderPath + FILE_SYSTEM_DIVIDER + fileName;
    return FileSystemUtils::isFilePresent(filePath);
}

void FSFileRepository::createHashFile()
{
    // Create the first column for the hash file.
    const auto headerLine =
      FILE_NAME_COLUMN + COLUMN_DIVIDER + FILE_HASH_COLUMN + COLUMN_DIVIDER + FILE_PATH_COLUMN + "\n";

    // Create the file
    FileSystemUtils::createFileWithContent(hashFilePath, headerLine);
}

void FSFileRepository::appendFileInfoIntoHashFile(const FileInfo& fileInfo)
{
    LOG(TRACE) << "FSFileRepository: Attempting to append FileInfo(" << fileInfo.name << ", " << fileInfo.hash << ", "
               << fileInfo.path << ") into the hash file.";

    // Check if the file exists
    const auto hashFilePath = m_folderPath + FILE_SYSTEM_DIVIDER + HASH_FILE;
    if (!FileSystemUtils::isFilePresent(hashFilePath))
    {
        LOG(TRACE) << "FSFileRepository: Hash file does not exist.";
        return;
    }

    // Append the line into file
    std::ofstream appender;
    appender.open(hashFilePath, std::ios_base::app);

    // Create the line that will be appended into the file
    const auto infoLine =
      fileInfo.name + FILE_SYSTEM_DIVIDER + fileInfo.hash + FILE_SYSTEM_DIVIDER + fileInfo.path + "\n";
    appender << infoLine;

    // And close the file appender
    appender.close();
}

std::string FSFileRepository::obtainFileHash(const std::string& fileName)
{
    // Read through the file

    return std::string();
}

std::string FSFileRepository::calculateFileHash(const std::string& filePath)
{
    // Check if the file exists
    if (!FileSystemUtils::isFilePresent(filePath))
        return "";

    // Load the entire content of the file, and get the hash.
    std::string stringContent;
    if (!FileSystemUtils::readFileContent(filePath, stringContent))
        return "";

    // Calculate the hash
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const uint8_t*>(stringContent.c_str()), stringContent.size(), hash);

    // Generate the hash string
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (const auto byte : hash)
        ss << std::setw(2) << static_cast<int>(byte);

    // Return the generated string
    return ss.str();
}
}    // namespace wolkabout
