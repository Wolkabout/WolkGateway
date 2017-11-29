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

#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <functional>
#include <string>

namespace wolkabout
{
class MqttClient
{
public:
    using OnMessageReceivedCallback = std::function<void(std::string topic, std::string message)>;

    virtual ~MqttClient() = default;

    virtual bool connect(const std::string& username, const std::string& password, const std::string& trustStore,
                         const std::string& address, const std::string& clientId) = 0;
    virtual void disconnect() = 0;

    virtual bool isConnected() = 0;

    virtual void setLastWill(const std::string& topic, const std::string& message) = 0;

    virtual bool subscribe(const std::string& topic) = 0;
    virtual bool publish(const std::string& topic, const std::string& message) = 0;

    void onMessageReceived(OnMessageReceivedCallback onMessageReceived);

protected:
    OnMessageReceivedCallback m_onMessageReceived;
};
}

#endif
