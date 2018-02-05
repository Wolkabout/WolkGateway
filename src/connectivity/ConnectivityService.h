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

#ifndef CONNECTIVITYSERVICE_H
#define CONNECTIVITYSERVICE_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class ConnectivityServiceListener
{
public:
    virtual ~ConnectivityServiceListener() = default;

	virtual void messageReceived(const std::string& topic, const std::string& message) = 0;

	virtual const std::vector<std::string>& getTopics() const = 0;
};

class OutboundMessage;
class ConnectivityService
{
public:
    virtual ~ConnectivityService() = default;

    virtual bool connect() = 0;
    virtual void disconnect() = 0;

	virtual bool isConnected() = 0;

    virtual bool publish(std::shared_ptr<OutboundMessage> outboundMessage) = 0;

    void setListener(std::weak_ptr<ConnectivityServiceListener> listener);

protected:
    std::weak_ptr<ConnectivityServiceListener> m_listener;
};
}

#endif
