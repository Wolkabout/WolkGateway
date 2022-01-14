/*
 * Copyright 2020 WolkAbout Technology s.r.o.
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

#include "GatewayUpdateService.h"

#include "OutboundMessageHandler.h"
#include "core/model/GatewayUpdateRequest.h"
#include "core/model/GatewayUpdateResponse.h"
#include "core/model/Message.h"
#include "core/protocol/RegistrationProtocol.h"
#include "core/utilities/Logger.h"
#include "repository/device/DeviceRepository.h"

#include <cassert>

namespace
{
static const short RETRY_COUNT = 3;
static const std::chrono::milliseconds RETRY_TIMEOUT{5000};
}    // namespace

namespace wolkabout
{
GatewayUpdateService::GatewayUpdateService(std::string gatewayKey, RegistrationProtocol& protocol,
                                           DeviceRepository& deviceRepository,
                                           OutboundMessageHandler& outboundPlatformMessageHandler)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_deviceRepository{deviceRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_platformRetryMessageHandler{outboundPlatformMessageHandler}
, m_pendingUpdateRequest{nullptr}
{
}

GatewayUpdateService::~GatewayUpdateService() = default;

void GatewayUpdateService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    m_platformRetryMessageHandler.messageReceived(message);

    if (m_protocol.isGatewayUpdateResponse(*message))
    {
        const auto response = m_protocol.makeGatewayUpdateResponse(*message);
        if (!response)
        {
            LOG(ERROR) << "GatewayUpdateService: Gateway update response could not be deserialized. Channel: '"
                       << message->getChannel() << "' Payload: '" << message->getContent() << "'";
            return;
        }

        handleUpdateResponse(*response);
    }
    else
    {
        LOG(WARN) << "GatewayUpdateService: Unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
    }
}

const Protocol& GatewayUpdateService::getProtocol() const
{
    return m_protocol;
}

void GatewayUpdateService::onGatewayUpdated(std::function<void()> callback)
{
    std::lock_guard<decltype(m_mutex)> lg{m_mutex};
    m_onGatewayUpdated = callback;
}

void GatewayUpdateService::updateGateway(const DetailedDevice& device)
{
    std::lock_guard<decltype(m_mutex)> lg{m_mutex};

    LOG(TRACE) << METHOD_INFO;

    auto savedGateway = m_deviceRepository.findByDeviceKey(device.getKey());
    auto newGateway =
      std::unique_ptr<DetailedDevice>(new DetailedDevice(device.getName(), device.getKey(), device.getTemplate()));
    if (savedGateway)
    {
        if (*savedGateway != *newGateway)
        {
            LOG(ERROR) << "GatewayUpdateService: Gateway update already performed, ignoring changes to device template";
            return;
        }

        LOG(INFO) << "GatewayUpdateService: Ignoring gateway update. Already registered with given device info and "
                     "device template";
        return;
    }

    LOG(INFO) << "GatewayUpdateService: Updating gateway";

    m_pendingUpdateRequest = std::move(newGateway);

    std::shared_ptr<Message> updateRequest =
      m_protocol.makeMessage(m_gatewayKey, GatewayUpdateRequest{*m_pendingUpdateRequest});
    if (!updateRequest)
    {
        LOG(WARN) << "GatewayUpdateService: Unable to create gateway update message";
        return;
    }

    auto responseChannel = m_protocol.getResponseChannel(m_gatewayKey, *updateRequest);
    RetryMessageStruct retryMessage{
      updateRequest, responseChannel,
      [=](std::shared_ptr<Message>) { LOG(ERROR) << "Failed to update gateway, no response from platform"; },
      RETRY_COUNT, RETRY_TIMEOUT};
    m_platformRetryMessageHandler.addMessage(retryMessage);
}

void GatewayUpdateService::handleUpdateResponse(const GatewayUpdateResponse& response)
{
    std::lock_guard<decltype(m_mutex)> lg{m_mutex};

    LOG(TRACE) << METHOD_INFO;

    if (!m_pendingUpdateRequest)
    {
        LOG(ERROR) << "GatewayUpdateService: Ignoring unexpected gateway update response";
        return;
    }

    const auto updateResult = response.getResult();
    if (updateResult.getCode() == PlatformResult::Code::OK ||
        updateResult.getCode() == PlatformResult::Code::ERROR_SUBDEVICE_MANAGEMENT_FORBIDDEN)
    {
        LOG(INFO) << "GatewayUpdateService: Gateway successfully update on platform";

        LOG(DEBUG) << "GatewayUpdateService: Saving gateway";
        m_deviceRepository.save(*m_pendingUpdateRequest);

        if (m_onGatewayUpdated)
        {
            m_onGatewayUpdated();
        }
    }
    else
    {
        LOG(ERROR) << "GatewayUpdateService: Unable to perform update gateway. Reason: '"
                   << response.getResult().getMessage() << "' Description: " << response.getResult().getDescription();
    }

    m_pendingUpdateRequest = nullptr;
}
}    // namespace wolkabout
