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

#include "PahoMqttClient.h"
#include "MqttClient.h"
#include "async_client.h"

#include <atomic>
#include <string>

namespace wolkabout
{
const unsigned short PahoMqttClient::MQTT_CONNECTION_COMPLETITION_TIMEOUT_MSEC = 2000;
const unsigned short PahoMqttClient::MQTT_ACTION_COMPLETITION_TIMEOUT_MSEC = 2000;
const unsigned short PahoMqttClient::MQTT_KEEP_ALIVE_SEC = 60;
const unsigned short PahoMqttClient::MQTT_QOS = 2;

PahoMqttClient::PahoMqttClient() : m_isConnected(false), m_lastWillTopic(""), m_lastWillMessage("") {}

bool PahoMqttClient::connect(const std::string& username, const std::string& password, const std::string& trustStore,
                             const std::string& host, const std::string& clientId)
{
    if (m_isConnected)
    {
        return true;
    }

    m_isConnected = false;
    m_client.reset(new mqtt::async_client(host, clientId));
    m_client->set_callback(*this);

    mqtt::connect_options connectOptions;
	connectOptions.set_user_name(username);
	connectOptions.set_password(password);
    connectOptions.set_clean_session(true);
    connectOptions.set_keep_alive_interval(MQTT_KEEP_ALIVE_SEC);

	mqtt::ssl_options sslOptions;
	sslOptions.set_enable_server_cert_auth(false);
	sslOptions.set_trust_store(trustStore);
	connectOptions.set_ssl(sslOptions);

    if (!m_lastWillTopic.empty() && !m_lastWillMessage.empty())
    {
        mqtt::will_options willOptions;
        willOptions.set_payload(m_lastWillMessage);
        willOptions.set_qos(MQTT_QOS);
        willOptions.set_retained(false);
        willOptions.set_topic(m_lastWillTopic);
        connectOptions.set_will(willOptions);
    }

    try
    {
        mqtt::token_ptr token = m_client->connect(connectOptions);
		token->wait_for(std::chrono::milliseconds(MQTT_CONNECTION_COMPLETITION_TIMEOUT_MSEC));

        if (!token->is_complete() || !m_isConnected)
        {
            return false;
        }
    }
    catch (mqtt::exception&)
    {
        return false;
    }

    return true;
}

void PahoMqttClient::disconnect()
{
    if (m_isConnected)
    {
        m_client->disconnect();
    }
}

bool PahoMqttClient::isConnected()
{
    return m_isConnected;
}

void PahoMqttClient::setLastWill(const std::string& topic, const std::string& message)
{
    m_lastWillTopic = topic;
    m_lastWillMessage = message;
}

bool PahoMqttClient::subscribe(const std::string& topic)
{
    if (!m_isConnected)
    {
        return false;
    }

    try
    {
        mqtt::token_ptr token = m_client->subscribe(topic, MQTT_QOS);
		token->wait_for(std::chrono::milliseconds(MQTT_ACTION_COMPLETITION_TIMEOUT_MSEC));

        if (!token->is_complete())
        {
            return false;
        }
    }
    catch (mqtt::exception&)
    {
        return false;
    }

    return true;
}

bool PahoMqttClient::publish(const std::string& topic, const std::string& message)
{
    if (!m_isConnected)
    {
        return false;
    }

	std::lock_guard<std::mutex> guard{m_mutex};

    try
	{
		//std::cout << "sending message: " << message << ", to: " << topic << std::endl;

        mqtt::message_ptr pubmsg = mqtt::make_message(topic, message.c_str(), strlen(message.c_str()));
        pubmsg->set_qos(MQTT_QOS);

        mqtt::token_ptr token = m_client->publish(pubmsg);
		token->wait_for(std::chrono::milliseconds(MQTT_ACTION_COMPLETITION_TIMEOUT_MSEC));

        if (!token->is_complete() || !m_isConnected)
        {
            return false;
        }
    }
    catch (mqtt::exception&)
    {
        return false;
    }

    return true;
}

void PahoMqttClient::connected(const mqtt::string& /* cause */)
{
    m_isConnected = true;
}

void PahoMqttClient::connection_lost(const mqtt::string& /* cause */)
{
    m_isConnected = false;
}

void PahoMqttClient::message_arrived(mqtt::const_message_ptr msg)
{
    m_onMessageReceived(msg->get_topic(), msg->get_payload_str());
}

void PahoMqttClient::delivery_complete(mqtt::delivery_token_ptr /* tok */) {}
}
