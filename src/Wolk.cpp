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

#include "Wolk.h"
#include "InboundMessageHandler.h"
#include "WolkBuilder.h"
#include "connectivity/ConnectivityService.h"
#include "model/Device.h"
#include "service/DataService.h"
#include "service/FirmwareUpdateService.h"
#include "service/PublishingService.h"

#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

namespace wolkabout
{
WolkBuilder Wolk::newBuilder(Device device)
{
    return WolkBuilder(device);
}

void Wolk::connect()
{
    connectToPlatform();
    connectToDevices();
}

void Wolk::disconnect()
{
    addToCommandBuffer([=]() -> void { m_platformConnectivityService->disconnect(); });
    addToCommandBuffer([=]() -> void { m_deviceConnectivityService->disconnect(); });
}

Wolk::Wolk(Device device) : m_device(device)
{
    m_commandBuffer = std::unique_ptr<CommandBuffer>(new CommandBuffer());
}

void Wolk::addToCommandBuffer(std::function<void()> command)
{
    m_commandBuffer->pushCommand(std::make_shared<std::function<void()>>(command));
}

unsigned long long Wolk::currentRtc()
{
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    return static_cast<unsigned long long>(std::chrono::duration_cast<std::chrono::seconds>(duration).count());
}

void Wolk::connectToPlatform()
{
    addToCommandBuffer([=]() -> void {
        if (m_platformConnectivityService->connect())
        {
            m_platformPublisher->connected();
        }
        else
        {
            connectToPlatform();
        }
    });
}

void Wolk::connectToDevices()
{
    addToCommandBuffer([=]() -> void {
        if (m_deviceConnectivityService->connect())
        {
            m_devicePublisher->connected();
        }
        else
        {
            connectToDevices();
        }
    });
}

Wolk::ConnectivityFacade::ConnectivityFacade(InboundMessageHandler& handler,
                                             std::function<void()> connectionLostHandler)
: m_messageHandler{handler}, m_connectionLostHandler{connectionLostHandler}
{
    m_messageHandler.onChannelsUpdated([&] { channelsUpdated(); });
}

void Wolk::ConnectivityFacade::messageReceived(const std::string& topic, const std::string& message)
{
    m_messageHandler.messageReceived(topic, message);
}

void Wolk::ConnectivityFacade::connectionLost()
{
    m_connectionLostHandler();
}

std::vector<std::string> Wolk::ConnectivityFacade::getTopics() const
{
    return m_messageHandler.getTopics();
}
}    // namespace wolkabout
