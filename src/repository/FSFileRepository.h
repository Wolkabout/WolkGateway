#ifndef WOLKGATEWAY_FSFILEREPOSITORY_H
#define WOLKGATEWAY_FSFILEREPOSITORY_H

#include "FileRepository.h"

namespace wolkabout
{
class FSFileRepository : public FileRepository
{
public:
    /**
     * Default constructor for the FileSystemFileRepository.
     *
     * @param folderPath The path to the folder which stores the files.
     */
    explicit FSFileRepository(std::string folderPath);

    /**
     * This method serves to return the information about a file.
     * This is an overridden method from `FileRepository`.
     *
     * @param fileName The name of the file we wish to gather information about.
     * @return The file info if the file with the name is found.
     */
    std::unique_ptr<FileInfo> getFileInfo(const std::string& fileName) override;

    /**
     * This method serves to return a list of files that are found locally.
     * This is an overridden method from `FileRepository`.
     *
     * @return The list of names of files that are found locally.
     */
    std::unique_ptr<std::vector<std::string>> getAllFileNames() override;

    /**
     * This method serves to store information about a file.
     * This is an overridden method from `FileRepository`.
     *
     * @param info The info struct that needs to be stored about files.
     */
    void store(const FileInfo& info) override;

    /**
     * This method removes the specific file with the name.
     * This is an overridden method from `FileRepository`.
     *
     * @param fileName The name of the file the user wishes to be removed.
     */
    void remove(const std::string& fileName) override;

    /**
     * This method removes all files that are locally found.
     * This is an overridden method from `FileRepository`.
     */
    void removeAll() override;

    /**
     * This method returns whether or not we can obtain information about a file.
     * This is an overridden method from `FileRepository`.
     *
     * @param fileName The name of the file the user wishes to get information.
     * @return Whether or not we can obtain info about a file.
     */
    bool containsInfoForFile(const std::string& fileName) override;

private:
    /**
     * This is an internal method that will create the hash file if it does not exist.
     */
    void createHashFile();

    /**
     * This is an internal method that will take the values from a FileInfo object and append a line with that info
     * into the hash file.
     *
     * @param fileInfo The file info object that needs to be appended into the file.
     */
    void appendFileInfoIntoHashFile(const FileInfo& fileInfo);

    /**
     * This is an internal method that will read the hash file and load the hash for it, or if it does not exist,
     * calculate it, and store it in the file.
     *
     * @param fileName The name of the file we request the hash for.
     * @return The hash value for the file (loaded from the hash file, or calculated just now).
     */
    std::string obtainFileHash(const std::string& fileName);

    /**
     * This is an internal method that explicitly obtains a SHA256 hash of a file.
     *
     * @param filePath The path to the file that we wish to find a SHA256 hash of.
     * @return The SHA256 of the file content.
     */
    static std::string calculateFileHash(const std::string& filePath);

    // This is where we store the path to the folder that is used for file management.
    std::string m_folderPath;
    std::string m_hashFilePath;
};
}    // namespace wolkabout

#endif    // WOLKGATEWAY_FSFILEREPOSITORY_H
