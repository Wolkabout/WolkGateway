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

#ifndef WOLKBUILDER_H
#define WOLKBUILDER_H

#include "ActuationHandler.h"
#include "ActuatorStatusProvider.h"
#include "model/Device.h"
#include "service/persist/PersistService.h"
#include "service/persist/json/JsonPersistService.h"

#include <functional>
#include <string>

namespace wolkabout
{
class Wolk;
class WolkBuilder final
{
public:
    ~WolkBuilder() = default;

    WolkBuilder(WolkBuilder&&) = default;

    /**
     * @brief WolkBuilder Initiates wolkabout::Wolk builder
     * @param device Device for which wolkabout::WolkBuilder is instantiated
     */
    WolkBuilder(Device device);

    /**
     * @brief Allows passing of URI to custom WolkAbout IoT platform instance
     * @param host Server URI
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& host(const std::string& host);

    /**
     * @brief Sets actuation handler
     * @param actuationHandler Lambda that handles actuation requests
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuationHandler(
      const std::function<void(const std::string& reference, const std::string& value)>& actuationHandler);

    /**
     * @brief Sets actuation handler
     * @param actuationHandler Instance of wolkabout::ActuationHandler
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuationHandler(std::weak_ptr<ActuationHandler> actuationHandler);

    /**
     * @brief Sets actuation status provider
     * @param actuatorStatusProvider Lambda that provides ActuatorStatus by reference of requested actuator
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuatorStatusProvider(
      const std::function<ActuatorStatus(const std::string& reference)>& actuatorStatusProvider);

    /**
     * @brief Sets actuation status provider
     * @param actuatorStatusProvider Instance of wolkabout::ActuatorStatusProvider
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& actuatorStatusProvider(std::weak_ptr<ActuatorStatusProvider> actuatorStatusProvider);

    /**
     * @brief Sets underlying persistence mechanism to be used<br>
     *        Sample filesystem persistence is used as default
     * @param persistService std::shared_ptr to wolkabout::PersistService implementation
     * @return Reference to current wolkabout::WolkBuilder instance (Provides fluent interface)
     */
    WolkBuilder& withDataPersistence(
      std::shared_ptr<PersistService> withDataPersistence = std::make_shared<wolkabout::JsonPersistService>());

    /**
     * @brief Builds Wolk instance
     * @return Wolk instance as std::unique_ptr<Wolk>
     */
    std::unique_ptr<Wolk> build() const;

    /**
     * @brief operator std::unique_ptr<Wolk> Conversion to wolkabout::wolk as result returns std::unique_ptr to built
     * wolkabout::Wolk instance
     */
    operator std::unique_ptr<Wolk>() const;

private:
    std::string m_host;
    Device m_device;

    std::function<void(std::string, std::string)> m_actuationHandlerLambda;
    std::weak_ptr<ActuationHandler> m_actuationHandler;

    std::function<ActuatorStatus(std::string)> m_actuatorStatusProviderLambda;
    std::weak_ptr<ActuatorStatusProvider> m_actuatorStatusProvider;

    std::shared_ptr<PersistService> m_persistService;

    static const constexpr char* WOLK_DEMO_HOST = "ssl://api-demo.wolkabout.com:8883";
};
}

#endif
