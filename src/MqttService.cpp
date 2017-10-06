/*
 * Copyright 2017 WolkAbout Technology s.r.o.
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

#include "MqttService.h"
#include "Device.h"
#include "async_client.h"

#include <iostream>
#include <memory>
#include <string>
#include <utility>

namespace wolkabout
{
MqttService::MqttService(Device device, std::string host)
: m_device(std::move(device))
, m_host(std::move(host))
, m_subscriptionList({})
, m_connected(false)
, m_mqttServiceListener(nullptr)
{
}

void MqttService::connect()
{
    if (m_connected)
    {
        return;
    }

    m_connected = false;
    m_mqttClient.reset(new mqtt::async_client(m_host, m_device.getDeviceKey()));
    m_mqttClient->set_callback(*this);

    mqtt::connect_options connectOptions;
    connectOptions.set_user_name(m_device.getDeviceKey());
    connectOptions.set_password(m_device.getDevicePassword());
    connectOptions.set_clean_session(false);
    connectOptions.set_keep_alive_interval(MQTT_KEEP_ALIVE_SEC);

    mqtt::will_options willOptions;
    willOptions.set_payload(std::string("Gone offline"));
    willOptions.set_qos(MQTT_QOS);
    willOptions.set_retained(false);
    willOptions.set_topic(std::string("lastwill/" + m_device.getDeviceKey()));
    connectOptions.set_will(willOptions);

    mqtt::ssl_options sslOptions;
    sslOptions.set_enable_server_cert_auth(false);
    sslOptions.set_trust_store(CERTIFICATE_NAME);
    connectOptions.set_ssl(sslOptions);

    try
    {
        mqtt::token_ptr connectToken = m_mqttClient->connect(connectOptions);
        connectToken->wait_for(MQTT_CONNECTION_COMPLETITION_TIMEOUT_MS);

        if (!connectToken->is_complete())
        {
            return;
        }

        if (!m_mqttClient->is_connected())
        {
            return;
        }

        for (const std::string& topic : m_subscriptionList)
        {
            m_mqttClient->subscribe(topic, MQTT_QOS);
        }
    }
    catch (mqtt::exception&)
    {
        return;
    }
}

void MqttService::disconnect()
{
    if (m_connected)
    {
        m_mqttClient->disconnect();
    }
}

MqttService& MqttService::setSubscriptionList(const std::vector<std::string>& subscriptionList)
{
    m_subscriptionList = subscriptionList;
    connect();

    return *this;
}

bool MqttService::isConnected() const
{
    return m_connected;
}

void MqttService::publish(const std::string& topic, const std::string& message)
{
    if (m_connected)
    {
        mqtt::message_ptr pubmsg = mqtt::make_message(topic, message.c_str(), strlen(message.c_str()));
        pubmsg->set_qos(MQTT_QOS);

        m_mqttClient->publish(pubmsg);
    }
    else
    {
        connect();
    }
}

void MqttService::setListener(MqttServiceListener* mqttServiceListener)
{
    m_mqttServiceListener = mqttServiceListener;
}

void MqttService::connected(const mqtt::string& /* cause */)
{
    m_connected = true;
}

void MqttService::connection_lost(const mqtt::string& /* cause */)
{
    m_connected = false;
}

void MqttService::message_arrived(mqtt::const_message_ptr msg)
{
    if (m_mqttServiceListener)
    {
        m_mqttServiceListener->messageArrived(msg->get_topic(), msg->get_payload_str());
    }
}
}
