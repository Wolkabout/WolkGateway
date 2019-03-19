/*
 * Copyright 2019 WolkAbout Technology s.r.o.
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
#include "model/GatewayUpdateRequest.h"
#include "model/GatewayUpdateResponse.h"
#include "model/Message.h"
#include "protocol/GatewaySubdeviceRegistrationProtocol.h"
#include "repository/DeviceRepository.h"
#include "utilities/Logger.h"

#include <cassert>

namespace
{
static const short RETRY_COUNT = 3;
static const std::chrono::milliseconds RETRY_TIMEOUT{5000};
}    // namespace

namespace wolkabout
{
GatewayUpdateService::GatewayUpdateService(std::string gatewayKey, GatewaySubdeviceRegistrationProtocol& protocol,
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

    if (!m_protocol.isMessageFromPlatform(*message))
    {
        LOG(WARN) << "GatewayUpdateService: Ignoring message on channel '" << message->getChannel()
                  << "'. Message not from platform.";
        return;
    }

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

const GatewayProtocol& GatewayUpdateService::getProtocol() const
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

    auto responseChannel = m_protocol.getResponseChannel(*updateRequest, m_gatewayKey, m_gatewayKey);
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
    if (updateResult == GatewayUpdateResponse::Result::OK)
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
        const auto updateFailureReason = [&]() -> std::string {
            if (updateResult == GatewayUpdateResponse::Result::ERROR_KEY_CONFLICT)
            {
                return "Device with given key already registered";
            }
            else if (updateResult == GatewayUpdateResponse::Result::ERROR_INVALID_DTO)
            {
                return "Rejected update DTO";
            }
            else if (updateResult == GatewayUpdateResponse::Result::ERROR_NOT_A_GATEWAY)
            {
                return "Device is not a gateway";
            }
            else if (updateResult == GatewayUpdateResponse::Result::ERROR_VALIDATION_ERROR)
            {
                return "Faulty update request";
            }
            else if (updateResult == GatewayUpdateResponse::Result::ERROR_KEY_MISSING)
            {
                return "Key missing from update request";
            }
            else if (updateResult == GatewayUpdateResponse::Result::ERROR_GATEWAY_NOT_FOUND)
            {
                return "Gateway has been deleted on platform";
            }
            else if (updateResult == GatewayUpdateResponse::Result::ERROR_SUBDEVICE_MANAGEMENT_CHANGE_NOT_ALLOWED)
            {
                return "Changing subdevice management is not allowed";
            }
            else if (updateResult == GatewayUpdateResponse::Result::ERROR_GATEWAY_UPDATE_FORBIDDEN)
            {
                return "Performing gateway update is not allowed more than once";
            }
            else if (updateResult == GatewayUpdateResponse::Result::ERROR_UNKNOWN)
            {
                return "Unknown gateway update error";
            }

            assert(false && "Unknown gateway update error");
            return "Unknown";
        }();

        LOG(ERROR) << "GatewayUpdateService: Unable to perform update gateway. Reason: '" << updateFailureReason
                   << "' Description: " << response.getDescription();
    }

    m_pendingUpdateRequest = nullptr;
}
}    // namespace wolkabout
