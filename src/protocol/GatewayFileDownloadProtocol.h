/*
 * Copyright 2018 WolkAbout Technology s.r.o.
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
#ifndef GATEWAYFILEDOWNLOADPROTOCOL_H
#define GATEWAYFILEDOWNLOADPROTOCOL_H

#include "protocol/GatewayProtocol.h"

#include <memory>

namespace wolkabout
{
class BinaryData;
class FilePacketRequest;
class Message;

class GatewayFileDownloadProtocol : public GatewayProtocol
{
public:
    virtual bool isBinary(const Message& message) const = 0;

    virtual std::unique_ptr<BinaryData> makeBinaryData(const Message& message) const = 0;

    virtual std::unique_ptr<Message> makeMessage(const std::string& gatewayKey, const std::string& deviceKey,
                                                 const FilePacketRequest& filePacketRequest) const = 0;

    inline Type getType() const override final { return GatewayProtocol::Type::FILE_DOWNLOAD; }
};
}    // namespace wolkabout

#endif    // GATEWAYFILEDOWNLOADPROTOCOL_H
