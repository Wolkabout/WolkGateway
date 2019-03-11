#ifndef FILEREPOSITORY_H
#define FILEREPOSITORY_H

#include "FileInfo.h"
#include "model/WolkOptional.h"

#include <vector>

namespace wolkabout
{
class FileRepository
{
public:
    virtual ~FileRepository() = default;

    virtual WolkOptional<FileInfo> getFileInfo(const std::string& fileName) = 0;
    virtual std::vector<FileInfo> getAllFileInfo() = 0;

    virtual void storeFileInfo(const FileInfo& info) = 0;
};
}    // namespace wolkabout

#endif    // FILEREPOSITORY_H
