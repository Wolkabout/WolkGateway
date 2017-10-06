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

#ifndef MQTTSERVICE_H
#define MQTTSERVICE_H

#include "Device.h"
#include "async_client.h"

#include <atomic>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class MqttServiceListener
{
public:
    virtual ~MqttServiceListener() = default;

    virtual void messageArrived(std::string topic, std::string message) = 0;
};

class MqttService : public mqtt::callback
{
public:
    MqttService(Device device, std::string host);
    virtual ~MqttService() = default;

    void connect();
    void disconnect();

    MqttService& setSubscriptionList(const std::vector<std::string>& subscriptionList = {});

    bool isConnected() const;

    void publish(const std::string& topic, const std::string& message);

    void setListener(MqttServiceListener* mqttServiceListener);

private:
    void connected(const mqtt::string& cause) override;
    void connection_lost(const mqtt::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;

    Device m_device;
    std::string m_host;

    std::vector<std::string> m_subscriptionList;

    std::atomic_bool m_connected;

    std::unique_ptr<mqtt::async_client> m_mqttClient;

    MqttServiceListener* m_mqttServiceListener;

    static const constexpr char* CERTIFICATE_NAME = "ca.crt";

    static const constexpr int MQTT_QOS = 2;
    static const constexpr int MQTT_KEEP_ALIVE_SEC = 60;
    static const constexpr unsigned int MQTT_CONNECTION_COMPLETITION_TIMEOUT_MS = 10000u;
};
}

#endif
