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

#include "WolkBuilder.h"
#include "ActuationHandler.h"
#include "ActuatorStatusProvider.h"
#include "Wolk.h"
#include "model/Device.h"
#include "service/connectivity/mqtt/MqttConnectivityService.h"
#include "service/connectivity/mqtt/PahoMqttClient.h"
#include "service/persist/PersistService.h"
#include "service/persist/json/JsonPersistService.h"

#include <functional>
#include <string>

namespace wolkabout
{
WolkBuilder& WolkBuilder::host(const std::string& host)
{
    m_host = host;
    return *this;
}

WolkBuilder& WolkBuilder::actuationHandler(
  const std::function<void(const std::string&, const std::string&)>& actuationHandler)
{
    m_actuationHandlerLambda = actuationHandler;
    m_actuationHandler.reset();
    return *this;
}

WolkBuilder& WolkBuilder::actuationHandler(std::weak_ptr<ActuationHandler> actuationHandler)
{
    m_actuationHandler = actuationHandler;
    m_actuationHandlerLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::actuatorStatusProvider(
  const std::function<ActuatorStatus(const std::string&)>& actuatorStatusProvider)
{
    m_actuatorStatusProviderLambda = actuatorStatusProvider;
    m_actuatorStatusProvider.reset();
    return *this;
}

WolkBuilder& WolkBuilder::actuatorStatusProvider(std::weak_ptr<ActuatorStatusProvider> actuatorStatusProvider)
{
    m_actuatorStatusProvider = actuatorStatusProvider;
    m_actuatorStatusProviderLambda = nullptr;
    return *this;
}

WolkBuilder& WolkBuilder::withDataPersistence(std::shared_ptr<PersistService> persistService)
{
    m_persistService = persistService;
    return *this;
}

std::unique_ptr<Wolk> WolkBuilder::build() const
{
    auto mqttClient = std::make_shared<PahoMqttClient>();
    auto connectivityService = std::make_shared<MqttConnectivityService>(mqttClient, m_device, m_host);

    auto publishService =
      std::make_shared<PublishService>(connectivityService, m_persistService, std::chrono::milliseconds(50));

    auto wolk = std::unique_ptr<Wolk>(new Wolk(publishService, m_device));

    wolk->m_actuationHandlerLambda = m_actuationHandlerLambda;
    wolk->m_actuationHandler = m_actuationHandler;

    wolk->m_actuatorStatusProviderLambda = m_actuatorStatusProviderLambda;
    wolk->m_actuatorStatusProvider = m_actuatorStatusProvider;

    connectivityService->setListener(
      [&](const ActuatorCommand& actuatorCommand) -> void { wolk->handleActuatorCommand(actuatorCommand); });

    return wolk;
}

wolkabout::WolkBuilder::operator std::unique_ptr<Wolk>() const
{
    return build();
}

WolkBuilder::WolkBuilder(Device device) : m_host(WOLK_DEMO_HOST), m_device(std::move(device)), m_persistService(nullptr)
{
}
}
