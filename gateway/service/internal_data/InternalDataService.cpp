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

#include "gateway/service/internal_data/InternalDataService.h"

#include "core/model/Device.h"
#include "core/model/Message.h"
#include "core/protocol/DataProtocol.h"
#include "core/utility/Logger.h"

#include <cassert>
#include <utility>

using namespace wolkabout::legacy;

namespace wolkabout::gateway
{
InternalDataService::InternalDataService(std::string gatewayKey, OutboundMessageHandler& platformOutboundHandler,
                                         OutboundMessageHandler& localOutboundHandler,
                                         GatewaySubdeviceProtocol& protocol)
: m_gatewayKey(std::move(gatewayKey))
, m_platformOutboundHandler(platformOutboundHandler)
, m_localOutboundHandler(localOutboundHandler)
, m_protocol(protocol)
{
}

void InternalDataService::messageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    // Parse it into a GatewaySubdeviceMessage
    auto parsedMessage =
      std::shared_ptr<Message>{m_protocol.makeOutboundMessage(m_gatewayKey, GatewaySubdeviceMessage{*message})};
    if (parsedMessage == nullptr)
    {
        LOG(ERROR) << "Failed to parse outgoing message from received local message.";
        return;
    }
    m_platformOutboundHandler.addMessage(parsedMessage);
}

const Protocol& InternalDataService::getProtocol()
{
    return m_protocol;
}

void InternalDataService::receiveMessages(const std::vector<GatewaySubdeviceMessage>& messages)
{
    LOG(TRACE) << METHOD_INFO;

    // Take every message, and reparse it into a local message, and send it out
    for (const auto& message : messages)
        m_localOutboundHandler.addMessage(std::make_shared<Message>(message.getMessage()));
}

std::vector<MessageType> InternalDataService::getMessageTypes() const
{
    return {MessageType::FEED_VALUES,
            MessageType::PARAMETER_SYNC,
            MessageType::TIME_SYNC,
            MessageType::FILE_UPLOAD_INIT,
            MessageType::FILE_UPLOAD_ABORT,
            MessageType::FILE_BINARY_RESPONSE,
            MessageType::FILE_URL_DOWNLOAD_INIT,
            MessageType::FILE_URL_DOWNLOAD_ABORT,
            MessageType::FILE_LIST_REQUEST,
            MessageType::FILE_DELETE,
            MessageType::FILE_PURGE,
            MessageType::FIRMWARE_UPDATE_INSTALL,
            MessageType::FIRMWARE_UPDATE_ABORT};
}
}    // namespace wolkabout::gateway
