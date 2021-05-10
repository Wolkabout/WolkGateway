#define private public
#define protected public
#include "repository/FSFileRepository.h"
#undef private
#undef protected

#include "core/utilities/FileSystemUtils.h"
#include "core/utilities/Logger.h"

#include <gtest/gtest.h>

class FSFileRepositoryTests : public ::testing::Test
{
public:
    const std::string FOLDER_PATH = "./test-folder";
    const std::string NON_EXISTING_FOLDER = "./non-existing-folder";
    const std::string FILE_SYSTEM_DIVIDER = "/";
    const std::string NON_EXISTING_FILE = "non-existing-file";
    const std::string UNREADABLE_FILE = "unreadable-file";
    const std::string TEST_FILE_NAME = "test-file";
    const std::string TEST_FILE_CONTENT = "Hello World!";
    const std::string TEST_FILE_HASH = "7f83b1657ff1fc53b92dc18148a1d65dfc2d4b1fa3d677284addd200126d9069";

    std::unique_ptr<wolkabout::FSFileRepository> fileRepository;

    static void SetUpTestCase()
    {
        // Create a logger
        wolkabout::Logger::init(wolkabout::LogLevel::TRACE, wolkabout::Logger::Type::CONSOLE);
    }

    void SetUp() override
    {
        // Create the folder
        wolkabout::FileSystemUtils::createDirectory(FOLDER_PATH);

        // Create the file repository
        fileRepository = std::unique_ptr<wolkabout::FSFileRepository>(new wolkabout::FSFileRepository(FOLDER_PATH));
    }

    void TearDown() override
    {
        // Clean up the repository
        fileRepository.reset();

        // Remove all the files in the folder
        for (const auto& fileInFolder : wolkabout::FileSystemUtils::listFiles(FOLDER_PATH))
        {
            wolkabout::FileSystemUtils::deleteFile(FOLDER_PATH + FILE_SYSTEM_DIVIDER + fileInFolder);
        }

        // Remove the folder
        wolkabout::FileSystemUtils::deleteFile(FOLDER_PATH);
    }
};

TEST_F(FSFileRepositoryTests, ConstructorTest)
{
    // Expect that the constructor will throw an error if the folder does not exist
    ASSERT_THROW(wolkabout::FSFileRepository(this->NON_EXISTING_FOLDER), std::runtime_error);

    // And that it won't on the existing one
    ASSERT_NO_THROW(wolkabout::FSFileRepository(this->FOLDER_PATH));
}

TEST_F(FSFileRepositoryTests, GetFileInfo_NonExistingFile)
{
    // In every case, obtaining info about a non existing file needs to return null.
    ASSERT_EQ(fileRepository->getFileInfo(NON_EXISTING_FILE), nullptr);
}

TEST_F(FSFileRepositoryTests, GetFileInfo_DummyFile)
{
    // Create the file
    const auto testFilePath = FOLDER_PATH + FILE_SYSTEM_DIVIDER + TEST_FILE_NAME;
    ASSERT_TRUE(wolkabout::FileSystemUtils::createFileWithContent(testFilePath, TEST_FILE_CONTENT));

    // Make place for the FileInfo object returned
    auto fileInfo = std::unique_ptr<wolkabout::FileInfo>();
    ASSERT_NO_THROW(fileInfo = fileRepository->getFileInfo(TEST_FILE_NAME));

    // And now analyze the returned value
    ASSERT_NE(fileInfo, nullptr);
    EXPECT_EQ(fileInfo->name, TEST_FILE_NAME);
    EXPECT_EQ(fileInfo->hash, TEST_FILE_HASH);
    EXPECT_EQ(fileInfo->path, testFilePath);
}

TEST_F(FSFileRepositoryTests, GetAllFileNames_EmptyVector)
{
    // Because the folder is empty, we should get back an vector containing no elements.
    ASSERT_EQ(fileRepository->getAllFileNames()->size(), 0);
}

TEST_F(FSFileRepositoryTests, GetAllFileNames_DummyFiles)
{
    // Create one dummy file
    ASSERT_TRUE(
      wolkabout::FileSystemUtils::createFileWithContent(FOLDER_PATH + FILE_SYSTEM_DIVIDER + "dummy-file-1", ""));

    // Expect that we would get one file
    std::unique_ptr<std::vector<std::string>> files;
    ASSERT_NO_THROW(files = fileRepository->getAllFileNames());
    EXPECT_EQ(files->size(), 1);
    EXPECT_EQ(files->at(0), "dummy-file-1");

    // Create another dummy file
    ASSERT_TRUE(
      wolkabout::FileSystemUtils::createFileWithContent(FOLDER_PATH + FILE_SYSTEM_DIVIDER + "dummy-file-2", ""));

    // Expect that we would get two files now
    ASSERT_NO_THROW(files = fileRepository->getAllFileNames());
    EXPECT_EQ(files->size(), 2);
    EXPECT_EQ(files->at(1), "dummy-file-2");

    // And that deleting the first dummy file returns us to 1
    ASSERT_TRUE(wolkabout::FileSystemUtils::deleteFile(FOLDER_PATH + FILE_SYSTEM_DIVIDER + "dummy-file-1"));

    // Expect that we would get one file now
    ASSERT_NO_THROW(files = fileRepository->getAllFileNames());
    EXPECT_EQ(files->size(), 1);
    EXPECT_EQ(files->at(0), "dummy-file-2");
}

TEST_F(FSFileRepositoryTests, Remove_NonExistingFile)
{
    // There should be no harm done if we try to remove a file that does not exist
    ASSERT_EQ(fileRepository->getAllFileNames()->size(), 0);
    ASSERT_NO_THROW(fileRepository->remove(NON_EXISTING_FILE));
}

TEST_F(FSFileRepositoryTests, Remove_DummyFile)
{
    // We need to create a file
    ASSERT_TRUE(wolkabout::FileSystemUtils::createFileWithContent(FOLDER_PATH + FILE_SYSTEM_DIVIDER + TEST_FILE_NAME,
                                                                  TEST_FILE_CONTENT));

    // See that we are actually having some files reported back
    ASSERT_EQ(fileRepository->getAllFileNames()->size(), 1);

    // And now remove it
    ASSERT_NO_THROW(fileRepository->remove(TEST_FILE_NAME));

    // Check that the repo is reporting empty again
    ASSERT_EQ(fileRepository->getAllFileNames()->size(), 0);
}

TEST_F(FSFileRepositoryTests, RemoveAll_EmptyDirectory)
{
    // There should be no harm done if we try to purge an empty directory
    ASSERT_NO_THROW(fileRepository->removeAll());
}

TEST_F(FSFileRepositoryTests, RemoveAll_TenDummyFiles)
{
    // Create ten files
    for (uint8_t i = 0; i < 10; ++i)
    {
        ASSERT_TRUE(wolkabout::FileSystemUtils::createFileWithContent(
          FOLDER_PATH + FILE_SYSTEM_DIVIDER + TEST_FILE_NAME + std::to_string(i), TEST_FILE_CONTENT));
    }

    // Check that we are actually reporting ten files
    ASSERT_EQ(fileRepository->getAllFileNames()->size(), 10);

    // And now purge them all
    ASSERT_NO_THROW(fileRepository->removeAll());

    // And now we should not have any reported
    ASSERT_EQ(fileRepository->getAllFileNames()->size(), 0);
}

TEST_F(FSFileRepositoryTests, ContainsInfoForFile_NonExistingFile)
{
    // This should just say false for a file that doesn't exist
    ASSERT_FALSE(fileRepository->containsInfoForFile(NON_EXISTING_FILE));
}

TEST_F(FSFileRepositoryTests, ContainerInfoForFile_DummyFile)
{
    // After we create a dummy file, it should report true
    ASSERT_TRUE(wolkabout::FileSystemUtils::createFileWithContent(FOLDER_PATH + FILE_SYSTEM_DIVIDER + TEST_FILE_NAME,
                                                                  TEST_FILE_CONTENT));
    ASSERT_TRUE(fileRepository->containsInfoForFile(TEST_FILE_NAME));
}

TEST_F(FSFileRepositoryTests, CalculateFileHash_FileDoesntExist)
{
    // Expect that it will return an empty string and pop out some messages
    ASSERT_EQ(wolkabout::FSFileRepository::calculateFileHash(FOLDER_PATH + FILE_SYSTEM_DIVIDER + NON_EXISTING_FILE),
              "");
}

TEST_F(FSFileRepositoryTests, DISABLED_CalculateFileHash_UnreadableFile)
{
    /**
     * Not for this test, it is disabled, as I can't make a file unreadable.
     * Making it unreadable with permissions, just makes it not present too in the eyes of
     * FileSystemUtils.
     */

    // Make the path for the file
    const auto unreadableFilePath = FOLDER_PATH + FILE_SYSTEM_DIVIDER + UNREADABLE_FILE;
    // If there is something there, delete it
    wolkabout::FileSystemUtils::deleteFile(unreadableFilePath);

    // Create the file that could not be read
    ASSERT_TRUE(wolkabout::FileSystemUtils::createFileWithContent(unreadableFilePath, ""));

    // Make it unreadable
    ASSERT_FALSE(chmod(unreadableFilePath.c_str(), 0));

    // Now attempt to calculate its hash
    EXPECT_EQ(wolkabout::FSFileRepository::calculateFileHash(unreadableFilePath), "");

    // And now delete the file
    wolkabout::FileSystemUtils::deleteFile(unreadableFilePath);
}

TEST_F(FSFileRepositoryTests, CalculateFileHash_RegularSmallFile)
{
    // Create the file that has some small amount of content in it.
    const auto testFilePath = FOLDER_PATH + FILE_SYSTEM_DIVIDER + TEST_FILE_NAME;
    ASSERT_TRUE(wolkabout::FileSystemUtils::createFileWithContent(testFilePath, TEST_FILE_CONTENT));

    // Compare the hash values
    EXPECT_EQ(wolkabout::FSFileRepository::calculateFileHash(testFilePath), TEST_FILE_HASH);

    // And delete the file
    wolkabout::FileSystemUtils::deleteFile(testFilePath);
}
