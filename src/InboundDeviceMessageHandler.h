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

#ifndef INBOUNDDEVICEMESSAGEHANDLER_H
#define INBOUNDDEVICEMESSAGEHANDLER_H

#include "InboundMessageHandler.h"
#include "utilities/CommandBuffer.h"

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

namespace wolkabout
{
class Message;

class DeviceMessageListener
{
public:
	virtual ~DeviceMessageListener() = default;
	virtual void deviceMessageReceived(std::shared_ptr<Message> message) = 0;
};

class InboundDeviceMessageHandler: public InboundMessageHandler
{
public:
	InboundDeviceMessageHandler();

	void messageReceived(const std::string& topic, const std::string& message) override;

	const std::vector<std::string>& getTopics() const override;

	template<class P>
	void setListener(std::weak_ptr<DeviceMessageListener> listener);

private:
	void addToCommandBuffer(std::function<void()> command);

	std::unique_ptr<CommandBuffer> m_commandBuffer;

	std::vector<std::string> m_subscriptionList;
	std::map<std::string, std::weak_ptr<DeviceMessageListener>> m_topicHandlers;

	mutable std::mutex m_lock;
};

template<class P>
void InboundDeviceMessageHandler::setListener(std::weak_ptr<DeviceMessageListener> listener)
{
	std::lock_guard<std::mutex> lg{m_lock};

	for(auto topic : P::getInstance().getDeviceTopics())
	{
		m_topicHandlers[topic] = listener;
		m_subscriptionList.push_back(topic);
	}

	channelsUpdated();
}
}

#endif
