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

#include "OutboundDataService.h"
#include "model/FirmwareUpdateResponse.h"
#include "connectivity/ConnectivityService.h"
#include "connectivity/json/OutboundMessageFactory.h"

#include <iostream>

namespace wolkabout
{
OutboundDataService::OutboundDataService(Device device, std::shared_ptr<ConnectivityService> connectivityService) :
	m_device{device}, m_connectivityService(std::move(connectivityService))
{
}

void OutboundDataService::addFirmwareUpdateResponse(const FirmwareUpdateResponse& response)
{
	const std::shared_ptr<OutboundMessage> outboundMessage =
			OutboundMessageFactory::make(m_device.getDeviceKey(), response);

	if (outboundMessage && m_connectivityService->publish(outboundMessage))
	{
		std::cout << "Message sent " << outboundMessage->getContent();
	}
}

void OutboundDataService::addFilePacketRequest(const FilePacketRequest& request)
{
	const std::shared_ptr<OutboundMessage> outboundMessage =
			OutboundMessageFactory::make(m_device.getDeviceKey(), request);

	if (outboundMessage && m_connectivityService->publish(outboundMessage))
	{
		std::cout << "Message sent " << outboundMessage->getContent();
	}
}

}
