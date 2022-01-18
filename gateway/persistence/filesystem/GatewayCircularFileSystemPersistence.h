/*
 * Copyright 2021 WolkAbout Technology s.r.o.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GATEWAYCIRCULARFILESYSTEMPERSISTENCE_H
#define GATEWAYCIRCULARFILESYSTEMPERSISTENCE_H

#include "gateway/persistence/filesystem/GatewayFilesystemPersistence.h"

namespace wolkabout
{
namespace gateway
{
/**
 * @brief The GatewayCircularFileSystemPersistence class
 * Specialization  of GatewayFilesystemPersistence for limited storage
 */
class GatewayCircularFileSystemPersistence : public GatewayFilesystemPersistence
{
public:
    explicit GatewayCircularFileSystemPersistence(const std::string& persistPath, PersistenceMethod method,
                                                  unsigned sizeLimitBytes = 0);

    bool push(std::shared_ptr<Message> message) override;
    void pop() override;

    void setSizeLimit(unsigned bytes);

private:
    void loadFileSize();
    void checkSizeAndNormalize();

    unsigned m_sizeLimitBytes;
    unsigned long long m_totalFileSize = 0;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // GATEWAYCIRCULARFILESYSTEMPERSISTENCE_H
