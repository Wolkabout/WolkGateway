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

#ifndef INBOUNDMODULEMESSAGEHANDLER_H
#define INBOUNDMODULEMESSAGEHANDLER_H

#include "InboundMessageHandler.h"
#include "model/Message.h"
#include "utilities/CommandBuffer.h"

#include <string>
#include <vector>

namespace wolkabout
{
class InboundModuleMessageHandler : public InboundMessageHandler
{
public:
    InboundModuleMessageHandler();

    void messageReceived(const std::string& topic, const std::string& message) override;

    const std::vector<std::string>& getTopics() const override;

    void setSensorReadingHandler(std::function<void(Message)> handler);
    void setAlarmHandler(std::function<void(Message)> handler);
    void setActuatorStatusHandler(std::function<void(Message)> handler);
    void setConfigurationHandler(std::function<void(Message)> handler);
    void setDeviceStatusHandler(std::function<void(Message)> handler);
    void setDeviceRegistrationRequestHandler(std::function<void(Message)> handler);
    void setDeviceReregistrationResponseHandler(std::function<void(Message)> handler);

    //	void setBinaryDataHandler(std::function<void(BinaryData)> handler);

    //	void setFirmwareUpdateCommandHandler(std::function<void(FirmwareUpdateCommand)> handler);

private:
    void addToCommandBuffer(std::function<void()> command);

    std::unique_ptr<CommandBuffer> m_commandBuffer;

    std::vector<std::string> m_subscriptionList;

    std::function<void(Message)> m_sensorReadingHandler;
    std::function<void(Message)> m_alarmHandler;
    std::function<void(Message)> m_actuationStatusHandler;
    std::function<void(Message)> m_configurationHandler;
    std::function<void(Message)> m_deviceStatusHandler;
    std::function<void(Message)> m_deviceRegistrationRequestHandler;
    std::function<void(Message)> m_deviceReregistrationResponseHandler;
    //	std::function<void(BinaryData)> m_binaryDataHandler;
    //	std::function<void(FirmwareUpdateCommand)> m_firmwareUpdateHandler;
};
}    // namespace wolkabout

#endif
