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

#ifndef PAHOMQTTCLIENT_H
#define PAHOMQTTCLIENT_H

#include "async_client.h"
#include "connectivity/mqtt/MqttClient.h"

#include <atomic>
#include <mutex>
#include <string>

namespace wolkabout
{
class PahoMqttClient : public MqttClient, public mqtt::callback
{
public:
    PahoMqttClient();
    virtual ~PahoMqttClient() = default;

    bool connect(const std::string& username, const std::string& password, const std::string& trustStore,
                 const std::string& host, const std::string& clientId) override;
    void disconnect() override;

    bool isConnected() override;

    void setLastWill(const std::string& topic, const std::string& message) override;

    bool subscribe(const std::string& topic) override;
    bool publish(const std::string& topic, const std::string& message) override;

private:
    std::atomic_bool m_isConnected;

    std::string m_lastWillTopic;
    std::string m_lastWillMessage;

    std::unique_ptr<mqtt::async_client> m_client;

    std::mutex m_mutex;

    static const unsigned short MQTT_CONNECTION_COMPLETITION_TIMEOUT_MSEC;
    static const unsigned short MQTT_ACTION_COMPLETITION_TIMEOUT_MSEC;
    static const unsigned short MQTT_KEEP_ALIVE_SEC;
    static const unsigned short MQTT_QOS;

    void connected(const mqtt::string& cause) override;
    void connection_lost(const mqtt::string& cause) override;
    void message_arrived(mqtt::const_message_ptr msg) override;
    void delivery_complete(mqtt::delivery_token_ptr tok) override;
};
}    // namespace wolkabout

#endif
