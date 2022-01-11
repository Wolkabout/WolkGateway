/*
 * Copyright 2021 WolkAbout Technology s.r.o.
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

#include "InternalDataService.h"

#include "core/model/Device.h"
#include "core/model/Message.h"
#include "core/protocol/DataProtocol.h"
#include "core/utilities/Logger.h"
#include "protocol/GatewayProtocol.h"
#include "repository/DeviceRepository.h"

#include <cassert>

namespace wolkabout
{
InternalDataService::InternalDataService(const std::string& gatewayKey, DataProtocol& protocol,
                                         GatewayProtocol& gatewayProtocol, DeviceRepository* deviceRepository,
                                         OutboundMessageHandler& outboundPlatformMessageHandler,
                                         OutboundMessageHandler& outboundDeviceMessageHandler,
                                         MessageListener* gatewayDevice)
: DataService(gatewayKey, protocol, gatewayProtocol, outboundPlatformMessageHandler, gatewayDevice)
, m_deviceRepository{deviceRepository}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
{
}

const GatewayProtocol& InternalDataService::getGatewayProtocol() const
{
    return m_gatewayProtocol;
}

void InternalDataService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::string channel = message->getChannel();

    // TODO do we filter or not?
    //    if (m_deviceRepository)
    //    {
    //        const std::string deviceKey = m_protocol.extractDeviceKeyFromChannel(channel);
    //        const std::unique_ptr<DetailedDevice> device = m_deviceRepository->findByDeviceKey(deviceKey);
    //        if (!device)
    //        {
    //            LOG(WARN) << "DataService: Not forwarding data message from device with key '" << deviceKey
    //                      << "'. Device not registered";
    //            return;
    //        }

    //        if (m_gatewayProtocol.isSensorReadingMessage(*message))
    //        {
    //            const std::string sensorReference = m_protocol.extractReferenceFromChannel(channel);
    //            const DeviceTemplate& deviceTemplate = device->getTemplate();
    //            if (!deviceTemplate.hasSensorTemplateWithReference(sensorReference))
    //            {
    //                LOG(WARN) << "DataService: Not forwarding sensor reading with reference '" << sensorReference
    //                          << "' from device with key '" << deviceKey
    //                          << "'. No sensor with given reference in device template";
    //                return;
    //            }
    //        }
    //        else if (m_gatewayProtocol.isAlarmMessage(*message))
    //        {
    //            const std::string alarmReference = m_protocol.extractReferenceFromChannel(channel);
    //            const DeviceTemplate& deviceTemplate = device->getTemplate();
    //            if (!deviceTemplate.hasAlarmTemplateWithReference(alarmReference))
    //            {
    //                LOG(WARN) << "DataService: Not forwarding alarm with reference '" << alarmReference
    //                          << "' from device with key '" << deviceKey
    //                          << "'. No event with given reference in device template";
    //                return;
    //            }
    //        }
    //        else if (m_gatewayProtocol.isActuatorStatusMessage(*message))
    //        {
    //            const std::string actuatorReference = m_protocol.extractReferenceFromChannel(channel);
    //            const DeviceTemplate& deviceTemplate = device->getTemplate();
    //            if (!deviceTemplate.hasActuatorTemplateWithReference(actuatorReference))
    //            {
    //                LOG(WARN) << "DataService: Not forwarding actuator status with reference '" << actuatorReference
    //                          << "' from device with key '" << deviceKey
    //                          << "'. No actuator with given reference in device template";
    //                return;
    //            }
    //        }
    //        else if (m_gatewayProtocol.isConfigurationCurrentMessage(*message))
    //        {
    //        }
    //        else
    //        {
    //            assert(false && "DataService: Unsupported message type");

    //            LOG(ERROR) << "DataService: Not forwarding message from device on channel: '" << channel
    //                       << "'. Unsupported message type";
    //            return;
    //        }
    //    }

    routeDeviceToPlatformMessage(message);
}

void InternalDataService::handleMessageForDevice(std::shared_ptr<Message> message)
{
    routePlatformToDeviceMessage(message);
}

void InternalDataService::routePlatformToDeviceMessage(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    const std::shared_ptr<Message> routedMessage = m_gatewayProtocol.routePlatformToDeviceMessage(*message);

    m_outboundDeviceMessageHandler.addMessage(routedMessage);
}
}    // namespace wolkabout
