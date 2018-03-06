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
#include "model/Message.h"
#include "utilities/Logger.h"

#include <algorithm>

namespace wolkabout
{
MqttConnectivityService::MqttConnectivityService(std::shared_ptr<MqttClient> mqttClient, const std::string& key,
                                                 const std::string& password, const std::string& host)
: m_mqttClient{std::move(mqttClient)}, m_key{key}, m_password{password}, m_host{host}
{
    m_mqttClient->onMessageReceived([this](std::string topic, std::string message) -> void {
        if (auto handler = m_listener.lock())
        {
            handler->messageReceived(topic, message);
        }
    });

    m_mqttClient->onConnectionLost([this]() -> void {
        if (auto handler = m_listener.lock())
        {
            handler->connectionLost();
        }
    });
}

bool MqttConnectivityService::connect()
{
    LOG(TRACE) << METHOD_INFO;

    LOG(INFO) << "MqttConnectivityService: Connecting to " << m_host;

    m_mqttClient->setLastWill(LAST_WILL_TOPIC_ROOT + m_key, "Gone offline");
    bool isConnected = m_mqttClient->connect(m_key, m_password, TRUST_STORE, m_host, "Gateway-" + m_key);
    if (isConnected)
    {
        LOG(INFO) << "MqttConnectivityService: Connected to " << m_host;

        if (auto handler = m_listener.lock())
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
    LOG(TRACE) << METHOD_INFO;

    LOG(INFO) << "MqttConnectivityService: Disconnecting from " << m_host;
    m_mqttClient->disconnect();
}

bool MqttConnectivityService::isConnected()
{
    LOG(TRACE) << METHOD_INFO;

    return m_mqttClient->isConnected();
}

bool MqttConnectivityService::publish(std::shared_ptr<Message> outboundMessage)
{
    LOG(TRACE) << METHOD_INFO;

    LOG(TRACE) << "MqttConnectivityService: Publishing message '" << outboundMessage->getContent() << "' on channel '"
               << outboundMessage->getChannel() << "' to " << m_host;

    return m_mqttClient->publish(outboundMessage->getChannel(), outboundMessage->getContent());
}
}    // namespace wolkabout
