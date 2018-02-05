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

#include "connectivity/mqtt/MqttConnectivityService.h"
#include "model/OutboundMessage.h"

namespace wolkabout
{
MqttConnectivityService::MqttConnectivityService(std::shared_ptr<MqttClient> mqttClient, Device device,
                                                 std::string host)
: m_mqttClient(std::move(mqttClient))
, m_device(std::move(device))
, m_host(std::move(host))
, m_connected(false)
{
	m_mqttClient->onMessageReceived([this](std::string topic, std::string message) -> void {
		if(auto handler = m_listener.lock())
		{
			handler->messageReceived(topic, message);
		}
	});
}

bool MqttConnectivityService::connect()
{
    m_mqttClient->setLastWill(LAST_WILL_TOPIC_ROOT + m_device.getDeviceKey(), "Gone offline");
    bool isConnected = m_mqttClient->connect(m_device.getDeviceKey(), m_device.getDevicePassword(), TRUST_STORE, m_host,
                                             m_device.getDeviceKey());
    if (isConnected)
    {
		if(auto handler = m_listener.lock())
		{
			const auto& topics = handler->getTopics();
			for (const std::string& topic : topics)
			{
				m_mqttClient->subscribe(topic);
			}
		}
    }

    return isConnected;
}

void MqttConnectivityService::disconnect()
{
    m_mqttClient->disconnect();
}

bool MqttConnectivityService::isConnected()
{
    return m_mqttClient->isConnected();
}

bool MqttConnectivityService::publish(std::shared_ptr<OutboundMessage> outboundMessage)
{
    return m_mqttClient->publish(outboundMessage->getTopic(), outboundMessage->getContent());
}
}
