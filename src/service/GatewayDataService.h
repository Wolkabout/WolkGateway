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

#ifndef GATEWAYDATASERVICE_H
#define GATEWAYDATASERVICE_H

#include "InboundMessageHandler.h"
#include "model/ActuatorStatus.h"
#include "model/ConfigurationItem.h"
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace wolkabout
{
class DataProtocol;
class Persistence;
class ConnectivityService;
class ConfigurationSetCommand;
class OutboundMessageHandler;

typedef std::function<void(const std::string&, const std::string&)> ActuatorSetHandler;
typedef std::function<void(const std::string&)> ActuatorGetHandler;

typedef std::function<void(const ConfigurationSetCommand&)> ConfigurationSetHandler;
typedef std::function<void()> ConfigurationGetHandler;

class GatewayDataService : public MessageListener
{
public:
    GatewayDataService(std::string deviceKey, DataProtocol& protocol, Persistence& persistence,
                       OutboundMessageHandler& outboundMessageHandler, const ActuatorSetHandler& actuatorSetHandler,
                       const ActuatorGetHandler& actuatorGetHandler,
                       const ConfigurationSetHandler& configurationSetHandler,
                       const ConfigurationGetHandler& configurationGetHandler);

    void messageReceived(std::shared_ptr<Message> message) override;
    const Protocol& getProtocol() override;

    void addSensorReading(const std::string& reference, const std::string& value, unsigned long long int rtc);

    void addSensorReading(const std::string& reference, const std::vector<std::string>& values,
                          unsigned long long int rtc);

    void addAlarm(const std::string& reference, bool active, unsigned long long int rtc);

    void addActuatorStatus(const std::string& reference, const std::string& value, ActuatorStatus::State state);

    void addConfiguration(const std::vector<ConfigurationItem>& configuration);

    void publishSensorReadings();

    void publishAlarms();

    void publishActuatorStatuses();

    void publishConfiguration();

private:
    std::string getSensorDelimiter(const std::string& key) const;

    void publishSensorReadingsForPersistanceKey(const std::string& persistanceKey);
    void publishAlarmsForPersistanceKey(const std::string& persistanceKey);
    void publishActuatorStatusesForPersistanceKey(const std::string& persistanceKey);
    void publishConfigurationForPersistanceKey(const std::string& persistanceKey);

    const std::string m_deviceKey;

    DataProtocol& m_protocol;
    Persistence& m_persistence;
    OutboundMessageHandler& m_outboundMessageHandler;

    ActuatorSetHandler m_actuatorSetHandler;
    ActuatorGetHandler m_actuatorGetHandler;

    ConfigurationSetHandler m_configurationSetHandler;
    ConfigurationGetHandler m_configurationGetHandler;

    std::map<std::string, std::string> m_sensorDelimiters;
    std::map<std::string, std::string> m_configurationDelimiters;

    static const constexpr unsigned int PUBLISH_BATCH_ITEMS_COUNT = 50;
};
}    // namespace wolkabout

#endif
