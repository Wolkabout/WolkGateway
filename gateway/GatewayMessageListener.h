/**
 * Copyright 2021 Wolkabout s.r.o.
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

#ifndef WOLKGATEWAY_GATEWAYMESSAGELISTENER_H
#define WOLKGATEWAY_GATEWAYMESSAGELISTENER_H

#include "core/Types.h"
#include "core/model/Message.h"

#include <memory>

namespace wolkabout
{
namespace gateway
{
/**
 * This interface is meant to represent an object that is capable of listening to incoming
 * gateway messages.
 */
class GatewayMessageListener
{
public:
    /**
     * Default virtual destructor.
     */
    virtual ~GatewayMessageListener() = default;

    /**
     * This is the method by which an object tells us which messages types it is interested in listening to.
     *
     * @return A list of message types.
     */
    virtual std::vector<MessageType> getMessageTypes() = 0;

    /**
     * This is the method by which an object receives messages that has been routed to it by the
     * GatewayInboundPlatformHandler.
     *
     * @param messages The received messages.
     */
    virtual void receiveMessages(std::vector<GatewaySubdeviceMessage> messages) = 0;
};
}    // namespace gateway
}    // namespace wolkabout

#endif    // WOLKGATEWAY_GATEWAYMESSAGELISTENER_H
