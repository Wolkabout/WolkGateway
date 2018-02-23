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

#ifndef INBOUNDPLATFORMMESSAGEHANDLER_H
#define INBOUNDPLATFORMMESSAGEHANDLER_H

#include "InboundMessageHandler.h"
#include "utilities/CommandBuffer.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>

namespace wolkabout
{
class Message;

class PlatformMessageListener
{
public:
	virtual ~PlatformMessageListener() = default;
	virtual void platformMessageReceived(std::shared_ptr<Message> message) = 0;
};

class InboundPlatformMessageHandler: public InboundMessageHandler
{
public:
	InboundPlatformMessageHandler(const std::string& gatewayKey);

	void messageReceived(const std::string& topic, const std::string& message) override;

	const std::vector<std::string>& getTopics() const override;

	template<class P>
	void setListener(std::weak_ptr<PlatformMessageListener> listener);

private:
	void addToCommandBuffer(std::function<void()> command);

	std::unique_ptr<CommandBuffer> m_commandBuffer;
	const std::string m_gatewayKey;

	std::vector<std::string> m_subscriptionList;

	std::map<std::string, std::weak_ptr<PlatformMessageListener>> m_topicHandlers;

	mutable std::mutex m_lock;
};

template<class P>
void InboundPlatformMessageHandler::setListener(std::weak_ptr<PlatformMessageListener> listener)
{
	std::lock_guard<std::mutex> lg{m_lock};

	for(auto topic : P::getInstance().getPlatformTopics())
	{
		m_topicHandlers[topic] = listener;
		m_subscriptionList.push_back(topic);
	}

	channelsUpdated();
}
}

#endif
