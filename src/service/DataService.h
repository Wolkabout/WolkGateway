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

#ifndef DATASERVICE_H
#define DATASERVICE_H

#include "DataServiceBase.h"
#include "OutboundMessageHandler.h"
#include "model/Message.h"
#include "utilities/Logger.h"
#include "utilities/StringUtils.h"
#include <string>
#include <vector>
#include <memory>

namespace wolkabout
{
template<class P>
class DataService: public DataServiceBase
{
public:
	DataService(const std::string& gatewayKey,
				std::shared_ptr<OutboundMessageHandler> outboundPlatformMessageHandler,
				std::shared_ptr<OutboundMessageHandler> outboundDeviceMessageHandler);

	void platformMessageReceived(std::shared_ptr<Message> message) override;

	void deviceMessageReceived(std::shared_ptr<Message> message) override;

private:
	void routeDeviceMessage(std::shared_ptr<Message> message);
	void routePlatformMessage(std::shared_ptr<Message> message);

	const std::string m_gatewayKey;

	std::shared_ptr<OutboundMessageHandler> m_outboundPlatformMessageHandler;
	std::shared_ptr<OutboundMessageHandler> m_outboundDeviceMessageHandler;
};


template<class P>
DataService<P>::DataService(const std::string& gatewayKey,
								   std::shared_ptr<OutboundMessageHandler> outboundPlatformMessageHandler,
								   std::shared_ptr<OutboundMessageHandler> outboundDeviceMessageHandler) :
	m_gatewayKey{gatewayKey},
	m_outboundPlatformMessageHandler{std::move(outboundPlatformMessageHandler)},
	m_outboundDeviceMessageHandler{std::move(outboundDeviceMessageHandler)}
{
}

template<class P>
void DataService<P>::platformMessageReceived(std::shared_ptr<Message> message)
{
	if(P::getInstance().isPlatformToGatewayMessage(message->getTopic(), m_gatewayKey))
	{
		// if message is for gateway device just resend it
		m_outboundDeviceMessageHandler->addMessage(message);
	}
	else if(P::getInstance().isPlatformToDeviceMessage(message->getTopic(), m_gatewayKey))
	{
		// if message is for device remove gateway info from channel
		routePlatformMessage(message);
	}
	else
	{
		LOG(WARN) << "Message channel not parsed: " << message->getTopic();
	}
}

template<class P>
void DataService<P>::deviceMessageReceived(std::shared_ptr<Message> message)
{
	if(P::getInstance().isGatewayToPlatformMessage(message->getTopic(), m_gatewayKey))
	{
		// if message is from gateway device just resend it
		m_outboundPlatformMessageHandler->addMessage(message);
	}
	else if(P::getInstance().isDeviceToPlatformMessage(message->getTopic()))
	{
		// if message is from device add gateway info to channel
		routeDeviceMessage(message);
	}
	else
	{
		LOG(WARN) << "Message channel not parsed: " << message->getTopic();
	}
}

template<class P>
void DataService<P>::routeDeviceMessage(std::shared_ptr<Message> message)
{
	const std::string topic = P::getInstance().routeDeviceMessage(message->getTopic(), m_gatewayKey);

	const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), topic)};

	m_outboundPlatformMessageHandler->addMessage(routedMessage);
}

template<class P>
void DataService<P>::routePlatformMessage(std::shared_ptr<Message> message)
{
	const std::string topic = P::getInstance().routePlatformMessage(message->getTopic(), m_gatewayKey);

	const std::shared_ptr<Message> routedMessage{new Message(message->getContent(), topic)};

	m_outboundDeviceMessageHandler->addMessage(routedMessage);
}

}

#endif
