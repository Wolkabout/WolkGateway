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

#include "service/SubdeviceRegistrationService.h"

#include "OutboundMessageHandler.h"
#include "core/model/DetailedDevice.h"
#include "core/model/Message.h"
#include "core/model/SubdeviceDeletionRequest.h"
#include "core/model/SubdeviceRegistrationRequest.h"
#include "core/model/SubdeviceRegistrationResponse.h"
#include "core/model/SubdeviceUpdateRequest.h"
#include "core/model/SubdeviceUpdateResponse.h"
#include "core/protocol/RegistrationProtocol.h"
#include "core/utilities/Logger.h"
#include "protocol/GatewaySubdeviceRegistrationProtocol.h"
#include "repository/DeviceRepository.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace
{
static const short RETRY_COUNT = 3;
static const std::chrono::milliseconds RETRY_TIMEOUT{5000};
}    // namespace

namespace wolkabout
{
SubdeviceRegistrationService::SubdeviceRegistrationService(std::string gatewayKey, RegistrationProtocol& protocol,
                                                           GatewaySubdeviceRegistrationProtocol& gatewayProtocol,
                                                           DeviceRepository& deviceRepository,
                                                           OutboundMessageHandler& outboundPlatformMessageHandler,
                                                           OutboundMessageHandler& outboundDeviceMessageHandler)
: m_gatewayKey{std::move(gatewayKey)}
, m_protocol{protocol}
, m_gatewayProtocol{gatewayProtocol}
, m_deviceRepository{deviceRepository}
, m_outboundPlatformMessageHandler{outboundPlatformMessageHandler}
, m_outboundDeviceMessageHandler{outboundDeviceMessageHandler}
, m_platformRetryMessageHandler{outboundPlatformMessageHandler}
{
}

SubdeviceRegistrationService::~SubdeviceRegistrationService() = default;

void SubdeviceRegistrationService::platformMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    m_platformRetryMessageHandler.messageReceived(message);

    if (m_protocol.isSubdeviceRegistrationResponse(*message))
    {
        const auto response = m_protocol.makeSubdeviceRegistrationResponse(*message);
        if (!response)
        {
            LOG(ERROR)
              << "SubdeviceRegistrationService: Device registration response could not be deserialized. Channel: '"
              << message->getChannel() << "' Payload: '" << message->getContent() << "'";
            return;
        }
        handleSubdeviceRegistrationResponse(response.get()->getSubdeviceKey(), *response);
    }
    else if (m_protocol.isSubdeviceUpdateResponse(*message))
    {
        const auto response = m_protocol.makeSubdeviceUpdateResponse(*message);
        if (!response)
        {
            LOG(ERROR) << "SubdeviceRegistrationService: Device update response could not be deserialized. Channel: '"
                       << message->getChannel() << "' Payload: '" << message->getContent() << "'";
            return;
        }
        handleSubdeviceUpdateResponse(response.get()->getSubdeviceKey(), *response);
    }
    else if (m_protocol.isSubdeviceDeletionResponse(*message))
    {
        LOG(INFO) << "SubdeviceRegistrationService: Received subdevice deletion response (" << message->getChannel()
                  << ")";
    }
    else
    {
        LOG(WARN) << "SubdeviceRegistrationService: Unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
    }
}

void SubdeviceRegistrationService::deviceMessageReceived(std::shared_ptr<Message> message)
{
    LOG(TRACE) << METHOD_INFO;

    if (m_gatewayProtocol.isSubdeviceRegistrationRequest(*message))
    {
        auto request = m_gatewayProtocol.makeSubdeviceRegistrationRequest(*message);
        if (!request)
        {
            LOG(ERROR)
              << "SubdeviceRegistrationService: Subdevice registration request could not be deserialized. Channel: '"
              << message->getChannel() << "' Payload: '" << message->getContent() << "'";
            return;
        }

        auto deviceKey = request->getSubdeviceKey();
        // This check is now intentionally disabled because the no forwarding of registration request when the gateway
        // is not updated is disabled, because the gateway never updates now.
        //        if (!m_deviceRepository.containsDeviceWithKey(m_gatewayKey) && deviceKey != m_gatewayKey)
        //        {
        //            addToPostponedSubdeviceRegistrationRequests(deviceKey, *request);
        //            return;
        //        }

        handleSubdeviceRegistrationRequest(deviceKey, *request);
    }
    else if (m_gatewayProtocol.isSubdeviceUpdateRequest(*message))
    {
        auto request = m_gatewayProtocol.makeSubdeviceUpdateRequest(*message);
        if (!request)
        {
            LOG(ERROR) << "SubdeviceRegistrationService: Subdevice update request could not be deserialized. Channel: '"
                       << message->getChannel() << "' Payload: '" << message->getContent() << "'";
            return;
        }

        auto deviceKey = request->getSubdeviceKey();
        // This check is now intentionally disabled because the no forwarding of registration request when the gateway
        // is not updated is disabled, because the gateway never updates now.
        //        if (!m_deviceRepository.containsDeviceWithKey(m_gatewayKey) && deviceKey != m_gatewayKey)
        //        {
        //            addToPostponedSubdeviceUpdateRequests(deviceKey, *request);
        //            return;
        //        }

        handleSubdeviceUpdateRequest(deviceKey, *request);
    }
    else
    {
        LOG(WARN) << "SubdeviceRegistrationService: unhandled message on channel '" << message->getChannel()
                  << "'. Unsupported message type";
        return;
    }
}

const GatewayProtocol& SubdeviceRegistrationService::getGatewayProtocol() const
{
    return m_gatewayProtocol;
}

const Protocol& SubdeviceRegistrationService::getProtocol() const
{
    return m_protocol;
}

void SubdeviceRegistrationService::onDeviceRegistered(
  std::function<void(const std::string& deviceKey)> onDeviceRegistered)
{
    m_onDeviceRegistered = onDeviceRegistered;
}

void SubdeviceRegistrationService::onDeviceUpdated(std::function<void(const std::string&)> onDeviceUpdated)
{
    m_onDeviceUpdated = onDeviceUpdated;
}

void SubdeviceRegistrationService::invokeOnDeviceRegisteredListener(const std::string& deviceKey) const
{
    if (m_onDeviceRegistered)
    {
        m_onDeviceRegistered(deviceKey);
    }
}

void SubdeviceRegistrationService::invokeOnDeviceUpdatedListener(const std::string& deviceKey) const
{
    if (m_onDeviceUpdated)
    {
        m_onDeviceUpdated(deviceKey);
    }
}

void SubdeviceRegistrationService::deleteDevicesOtherThan(const std::vector<std::string>& devicesKeys)
{
    const auto deviceKeysFromRepository = m_deviceRepository.findAllDeviceKeys();
    for (const std::string& deviceKeyFromRepository : *deviceKeysFromRepository)
    {
        if (std::find(devicesKeys.begin(), devicesKeys.end(), deviceKeyFromRepository) == devicesKeys.end())
        {
            if (deviceKeyFromRepository == m_gatewayKey)
            {
                LOG(DEBUG) << "Skiping delete gateway";
                continue;
            }

            LOG(INFO) << "Deleting device with key " << deviceKeyFromRepository;
            m_deviceRepository.remove(deviceKeyFromRepository);

            std::shared_ptr<Message> subdeviceDeletionRequestMessage =
              m_protocol.makeMessage(m_gatewayKey, SubdeviceDeletionRequest{deviceKeyFromRepository});
            if (!subdeviceDeletionRequestMessage)
            {
                LOG(WARN) << "SubdeviceRegistrationService: Unable to create deletion request message";
                continue;
            }

            auto responseChannel = m_protocol.getResponseChannel(m_gatewayKey, *subdeviceDeletionRequestMessage);
            RetryMessageStruct retryMessage{subdeviceDeletionRequestMessage, responseChannel,
                                            [=](std::shared_ptr<Message>) {
                                                LOG(ERROR)
                                                  << "Failed to delete device with key: " << deviceKeyFromRepository
                                                  << ", no response from platform";
                                            },
                                            RETRY_COUNT, RETRY_TIMEOUT};
            m_platformRetryMessageHandler.addMessage(retryMessage);
        }
    }
}

void SubdeviceRegistrationService::registerPostponedDevices()
{
    std::lock_guard<decltype(m_devicesWithPostponedRegistrationMutex)> devicesWithPostponedRegistrationLock(
      m_devicesWithPostponedRegistrationMutex);
    if (!m_devicesWithPostponedRegistration.empty())
    {
        LOG(INFO) << "SubdeviceRegistrationService: Processing postponed device registration requests";

        for (const auto& deviceWithPostponedRegistration : m_devicesWithPostponedRegistration)
        {
            handleSubdeviceRegistrationRequest(deviceWithPostponedRegistration.first,
                                               *deviceWithPostponedRegistration.second);
        }

        m_devicesWithPostponedRegistration.clear();
    }
}

void SubdeviceRegistrationService::updatePostponedDevices()
{
    std::lock_guard<decltype(m_devicesWithPostponedUpdateMutex)> devicesWithPostponedRegistrationLock(
      m_devicesWithPostponedUpdateMutex);
    if (!m_devicesWithPostponedUpdate.empty())
    {
        LOG(INFO) << "SubdeviceRegistrationService: Processing postponed device update requests";

        for (const auto& deviceWithPostponedUpdate : m_devicesWithPostponedUpdate)
        {
            handleSubdeviceUpdateRequest(deviceWithPostponedUpdate.first, *deviceWithPostponedUpdate.second);
        }

        m_devicesWithPostponedUpdate.clear();
    }
}

void SubdeviceRegistrationService::handleSubdeviceRegistrationRequest(const std::string& deviceKey,
                                                                      const SubdeviceRegistrationRequest& request)
{
    LOG(TRACE) << METHOD_INFO;

    if (deviceKey == m_gatewayKey)
    {
        LOG(ERROR) << "SubdeviceRegistrationService: Skipping registration of gateway";
        return;
    }

    LOG(INFO) << "SubdeviceRegistrationService: Handling registration request for device with key '" << deviceKey
              << "'";

    auto savedDevice = m_deviceRepository.findByDeviceKey(deviceKey);
    auto subdeviceRequestingRegistration = std::unique_ptr<DetailedDevice>(
      new DetailedDevice(request.getSubdeviceName(), request.getSubdeviceKey(), request.getTemplate()));
    if (savedDevice && *savedDevice == *subdeviceRequestingRegistration)
    {
        LOG(WARN) << "SubdeviceRegistrationService: Ignoring device registration request for device with key '"
                  << deviceKey << "'. Already registered with given device info and device template";
        return;
    }

    std::lock_guard<decltype(m_devicesAwaitingRegistrationResponseMutex)> l{m_devicesAwaitingRegistrationResponseMutex};
    auto device = std::unique_ptr<DetailedDevice>(
      new DetailedDevice(request.getSubdeviceName(), request.getSubdeviceKey(), request.getTemplate()));
    m_devicesAwaitingRegistrationResponse[deviceKey] = std::move(device);

    std::shared_ptr<Message> registrationRequest = m_protocol.makeMessage(m_gatewayKey, request);
    if (!registrationRequest)
    {
        LOG(WARN) << "SubdeviceRegistrationService: Unable to create registration request message";
        return;
    }

    auto responseChannel = m_protocol.getResponseChannel(m_gatewayKey, *registrationRequest);
    RetryMessageStruct retryMessage{registrationRequest, responseChannel,
                                    [=](std::shared_ptr<Message>) {
                                        LOG(ERROR) << "Failed to register device with key: " << deviceKey
                                                   << ", no response from platform";
                                    },
                                    RETRY_COUNT, RETRY_TIMEOUT};
    m_platformRetryMessageHandler.addMessage(retryMessage);
}

void SubdeviceRegistrationService::handleSubdeviceRegistrationResponse(const std::string& deviceKey,
                                                                       const SubdeviceRegistrationResponse& response)
{
    LOG(TRACE) << METHOD_INFO;

    if (deviceKey == m_gatewayKey)
    {
        LOG(ERROR) << "SubdeviceRegistrationService: Ignoring registration response for gateway";
        return;
    }

    std::lock_guard<decltype(m_devicesAwaitingRegistrationResponseMutex)> l(m_devicesAwaitingRegistrationResponseMutex);
    if (m_devicesAwaitingRegistrationResponse.find(deviceKey) == m_devicesAwaitingRegistrationResponse.end())
    {
        LOG(ERROR)
          << "SubdeviceRegistrationService: Ignoring unexpected device registration response for device with key '"
          << deviceKey << "'";
        return;
    }

    const auto registrationResult = response.getResult();
    if (registrationResult.getCode() == PlatformResult::Code::OK)
    {
        LOG(INFO) << "SubdeviceRegistrationService: Device with key '" << deviceKey
                  << "' successfully registered on platform";

        const auto& device = *m_devicesAwaitingRegistrationResponse.at(deviceKey);
        LOG(DEBUG) << "SubdeviceRegistrationService: Saving device with key '" << device.getKey()
                   << "' to device repository";

        m_deviceRepository.save(device);
        invokeOnDeviceRegisteredListener(deviceKey);
    }
    else
    {
        LOG(ERROR) << "SubdeviceRegistrationService: Unable to register device with key '" << deviceKey
                   << "'. Reason: '" << response.getResult().getMessage()
                   << "' Description: " << response.getResult().getDescription();
    }

    m_devicesAwaitingRegistrationResponse.erase(deviceKey);

    // send response to device
    std::shared_ptr<Message> registrationResponseMessage = m_gatewayProtocol.makeMessage(response);
    if (!registrationResponseMessage)
    {
        LOG(WARN) << "SubdeviceRegistrationService: Unable to create registration response message";
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(registrationResponseMessage);
}

void SubdeviceRegistrationService::handleSubdeviceUpdateRequest(const std::string& deviceKey,
                                                                const SubdeviceUpdateRequest& request)
{
    LOG(TRACE) << METHOD_INFO;

    if (deviceKey == m_gatewayKey)
    {
        LOG(ERROR) << "SubdeviceRegistrationService: Skipping update of gateway";
        return;
    }

    LOG(INFO) << "SubdeviceRegistrationService: Handling update request for device with key '" << deviceKey << "'";

    auto savedDevice = m_deviceRepository.findByDeviceKey(deviceKey);

    if (!savedDevice)
    {
        LOG(WARN) << "SubdeviceRegistrationService: Ignoring device update request for device with key '" << deviceKey
                  << "'. Device is not registered";
        return;
    }

    if (containsSubset(savedDevice->getTemplate().getAlarms(), request.getAlarms()) &&
        containsSubset(savedDevice->getTemplate().getSensors(), request.getSensors()) &&
        containsSubset(savedDevice->getTemplate().getActuators(), request.getActuators()) &&
        containsSubset(savedDevice->getTemplate().getConfigurations(), request.getConfigurations()))
    {
        LOG(WARN) << "SubdeviceRegistrationService: Ignoring device update request for device with key '" << deviceKey
                  << "'. Already updated device with given assets";
        return;
    }

    std::lock_guard<decltype(m_devicesAwaitingUpdateResponseMutex)> l{m_devicesAwaitingUpdateResponseMutex};
    auto device = std::unique_ptr<DeviceTemplate>(new DeviceTemplate(request.getConfigurations(), request.getSensors(),
                                                                     request.getAlarms(), request.getActuators()));
    m_devicesAwaitingUpdateResponse[deviceKey] = std::move(device);

    std::shared_ptr<Message> updateRequest = m_protocol.makeMessage(m_gatewayKey, request);
    if (!updateRequest)
    {
        LOG(WARN) << "SubdeviceRegistrationService: Unable to create update request message";
        return;
    }

    auto responseChannel = m_protocol.getResponseChannel(m_gatewayKey, *updateRequest);
    RetryMessageStruct retryMessage{updateRequest, responseChannel,
                                    [=](std::shared_ptr<Message>) {
                                        LOG(ERROR) << "Failed to update device with key: " << deviceKey
                                                   << ", no response from platform";
                                    },
                                    RETRY_COUNT, RETRY_TIMEOUT};
    m_platformRetryMessageHandler.addMessage(retryMessage);
}

void SubdeviceRegistrationService::handleSubdeviceUpdateResponse(const std::string& deviceKey,
                                                                 const SubdeviceUpdateResponse& response)
{
    LOG(TRACE) << METHOD_INFO;

    if (deviceKey == m_gatewayKey)
    {
        LOG(ERROR) << "SubdeviceRegistrationService: Ignoring update response for gateway";
        return;
    }

    std::lock_guard<decltype(m_devicesAwaitingUpdateResponseMutex)> l(m_devicesAwaitingUpdateResponseMutex);
    if (m_devicesAwaitingUpdateResponse.find(deviceKey) == m_devicesAwaitingUpdateResponse.end())
    {
        LOG(ERROR) << "SubdeviceRegistrationService: Ignoring unexpected device update response for device with key '"
                   << deviceKey << "'";
        return;
    }

    const auto registrationResult = response.getResult();
    if (registrationResult.getCode() == PlatformResult::Code::OK)
    {
        LOG(INFO) << "SubdeviceRegistrationService: Device with key '" << deviceKey
                  << "' successfully updated on platform";

        const auto& deviceTemplate = *m_devicesAwaitingUpdateResponse.at(deviceKey);
        LOG(DEBUG) << "SubdeviceRegistrationService: Saving device with key '" << deviceKey << "' to device repository";

        auto savedDevice = m_deviceRepository.findByDeviceKey(deviceKey);

        if (!savedDevice)
        {
            LOG(WARN) << "SubdeviceRegistrationService: Updated device not found in database";

            savedDevice.reset(new DetailedDevice{"", deviceKey, DeviceTemplate{}});
        }

        auto deviceToUpdate = *savedDevice;

        addAssetsToDevice(deviceToUpdate, deviceTemplate);

        m_deviceRepository.save(deviceToUpdate);
        invokeOnDeviceUpdatedListener(deviceKey);
    }
    else
    {
        LOG(ERROR) << "SubdeviceRegistrationService: Unable to update device with key '" << deviceKey << "'. Reason: '"
                   << response.getResult().getMessage() << "' Description: " << response.getResult().getDescription();
    }

    m_devicesAwaitingUpdateResponse.erase(deviceKey);

    // send response to device
    std::shared_ptr<Message> updateResponseMessage = m_gatewayProtocol.makeMessage(response);
    if (!updateResponseMessage)
    {
        LOG(WARN) << "SubdeviceRegistrationService: Unable to create update response message";
        return;
    }

    m_outboundDeviceMessageHandler.addMessage(updateResponseMessage);
}

void SubdeviceRegistrationService::addToPostponedSubdeviceRegistrationRequests(
  const std::string& deviceKey, const wolkabout::SubdeviceRegistrationRequest& request)
{
    LOG(TRACE) << METHOD_INFO;

    LOG(INFO) << "SubdeviceRegistrationService: Postponing registration of device with key '" << deviceKey
              << "'. Waiting for gateway to be updated";

    std::lock_guard<decltype(m_devicesWithPostponedRegistrationMutex)> l(m_devicesWithPostponedRegistrationMutex);
    auto postponedDeviceRegistration =
      std::unique_ptr<SubdeviceRegistrationRequest>(new SubdeviceRegistrationRequest(request));
    m_devicesWithPostponedRegistration[deviceKey] = std::move(postponedDeviceRegistration);
}

void SubdeviceRegistrationService::addToPostponedSubdeviceUpdateRequests(const std::string& deviceKey,
                                                                         const SubdeviceUpdateRequest& request)
{
    LOG(TRACE) << METHOD_INFO;

    LOG(INFO) << "SubdeviceRegistrationService: Postponing update of device with key '" << deviceKey
              << "'. Waiting for gateway to be updated";

    std::lock_guard<decltype(m_devicesWithPostponedUpdateMutex)> l(m_devicesWithPostponedUpdateMutex);
    auto postponedDeviceUpdate = std::unique_ptr<SubdeviceUpdateRequest>(new SubdeviceUpdateRequest(request));
    m_devicesWithPostponedUpdate[deviceKey] = std::move(postponedDeviceUpdate);
}

void SubdeviceRegistrationService::addAssetsToDevice(DetailedDevice& device, const DeviceTemplate& assets)
{
    auto existingTemplate = device.getTemplate();

    for (const auto& alarm : assets.getAlarms())
    {
        if (!containsSubset(device.getTemplate().getAlarms(), {alarm}))
        {
            existingTemplate.addAlarm(alarm);
        }
    }

    for (const auto& sensor : assets.getSensors())
    {
        if (!containsSubset(device.getTemplate().getSensors(), {sensor}))
        {
            existingTemplate.addSensor(sensor);
        }
    }

    for (const auto& actuator : assets.getActuators())
    {
        if (!containsSubset(device.getTemplate().getActuators(), {actuator}))
        {
            existingTemplate.addActuator(actuator);
        }
    }

    for (const auto& configuration : assets.getConfigurations())
    {
        if (!containsSubset(device.getTemplate().getConfigurations(), {configuration}))
        {
            existingTemplate.addConfiguration(configuration);
        }
    }

    device = DetailedDevice{device.getName(), device.getKey(), device.getPassword(), existingTemplate};
}

template bool SubdeviceRegistrationService::containsSubset<AlarmTemplate>(const std::vector<AlarmTemplate>& assets,
                                                                          const std::vector<AlarmTemplate>& subset);

template bool SubdeviceRegistrationService::containsSubset<SensorTemplate>(const std::vector<SensorTemplate>& assets,
                                                                           const std::vector<SensorTemplate>& subset);

template bool SubdeviceRegistrationService::containsSubset<ActuatorTemplate>(
  const std::vector<ActuatorTemplate>& assets, const std::vector<ActuatorTemplate>& subset);

template bool SubdeviceRegistrationService::containsSubset<ConfigurationTemplate>(
  const std::vector<ConfigurationTemplate>& assets, const std::vector<ConfigurationTemplate>& subset);

template <typename T>
bool SubdeviceRegistrationService::containsSubset(const std::vector<T>& assets, const std::vector<T>& subset)
{
    for (const auto& item : subset)
    {
        bool found = false;

        for (const auto& asset : assets)
        {
            if (asset == item)
            {
                found = true;
                break;
            }
        }

        if (!found)
        {
            return false;
        }
    }

    return true;
}

}    // namespace wolkabout
