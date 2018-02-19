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
#include "ActuationHandler.h"
#include "ActuatorStatusProvider.h"
#include "WolkBuilder.h"
#include "connectivity/ConnectivityService.h"
#include "service/FirmwareUpdateService.h"
#include "model/ActuatorStatus.h"
#include "model/Alarm.h"
#include "model/Device.h"
#include "model/SensorReading.h"
#include "InboundMessageHandler.h"
#include "service/DataService.h"

#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <utility>

#define INSTANTIATE_ADD_SENSOR_READING_FOR(x) \
    template void Wolk::addSensorReading<x>(const std::string& reference, x value, unsigned long long rtc)

namespace wolkabout
{
WolkBuilder Wolk::newBuilder(Device device)
{
    return WolkBuilder(device);
}

template <> void Wolk::addSensorReading(const std::string& reference, std::string value, unsigned long long rtc)
{
    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    auto sensorReading = std::make_shared<SensorReading>(value, reference, rtc);

    addToCommandBuffer(
	  [=]() -> void { m_dataService->addSensorReadings({sensorReading}); });
}

template <typename T> void Wolk::addSensorReading(const std::string& reference, T value, unsigned long long rtc)
{
    addSensorReading(reference, std::to_string(value), rtc);
}

template <> void Wolk::addSensorReading(const std::string& reference, bool value, unsigned long long rtc)
{
    addSensorReading(reference, std::string(value ? "true" : "false"), rtc);
}

template <> void Wolk::addSensorReading(const std::string& reference, char* value, unsigned long long rtc)
{
    addSensorReading(reference, std::string(value), rtc);
}

template <> void Wolk::addSensorReading(const std::string& reference, const char* value, unsigned long long rtc)
{
    addSensorReading(reference, std::string(value), rtc);
}

INSTANTIATE_ADD_SENSOR_READING_FOR(float);
INSTANTIATE_ADD_SENSOR_READING_FOR(double);
INSTANTIATE_ADD_SENSOR_READING_FOR(signed int);
INSTANTIATE_ADD_SENSOR_READING_FOR(signed long int);
INSTANTIATE_ADD_SENSOR_READING_FOR(signed long long int);
INSTANTIATE_ADD_SENSOR_READING_FOR(unsigned int);
INSTANTIATE_ADD_SENSOR_READING_FOR(unsigned long int);
INSTANTIATE_ADD_SENSOR_READING_FOR(unsigned long long int);

void Wolk::addAlarm(const std::string& reference, const std::string& value, unsigned long long rtc)
{
    if (rtc == 0)
    {
        rtc = Wolk::currentRtc();
    }

    auto alarm = std::make_shared<Alarm>(value, reference, rtc);

	addToCommandBuffer([=]() -> void { m_dataService->addAlarms({alarm}); });
}

void Wolk::publishActuatorStatus(const std::string& reference)
{
    const ActuatorStatus actuatorStatus = [&]() -> ActuatorStatus {
        if (auto provider = m_actuatorStatusProvider.lock())
        {
            return provider->getActuatorStatus(reference);
        }
        else if (m_actuatorStatusProviderLambda)
        {
            return m_actuatorStatusProviderLambda(reference);
        }

        return ActuatorStatus();
    }();

    auto actuatorStatusWithRef =
      std::make_shared<ActuatorStatus>(actuatorStatus.getValue(), reference, actuatorStatus.getState());
	addToCommandBuffer([=]() -> void { m_dataService->addActuatorStatus(actuatorStatusWithRef); });
}

void Wolk::connect()
{
    addToCommandBuffer([=]() -> void {
		if(!m_wolkConnectivityService->connect())
		{
			return;
		}

//		publishFirmwareVersion();
//		m_firmwareUpdateService->reportFirmwareUpdateResult();

		for (const std::string& actuatorReference : m_device.getActuatorReferences())
		{
			publishActuatorStatus(actuatorReference);
		}

		//publish();
    });
}

void Wolk::disconnect()
{
	addToCommandBuffer([=]() -> void { m_wolkConnectivityService->disconnect(); });
}

//void Wolk::publish()
//{
//    addToCommandBuffer([=]() -> void {
//        publishActuatorStatuses();
//        publishAlarms();
//        publishSensorReadings();

//        if (!m_persistence->isEmpty())
//        {
//            publish();
//        }
//    });
//}

Wolk::Wolk(std::shared_ptr<ConnectivityService> wolkConnectivityService,
		   std::shared_ptr<ConnectivityService> moduleConnectivityService,
		   std::shared_ptr<Persistence> persistence,
		   std::shared_ptr<InboundWolkaboutMessageHandler> inboundWolkaboutMessageHandler,
		   std::shared_ptr<InboundModuleMessageHandler> inboundModuleMessageHandler,
		   std::shared_ptr<OutboundServiceDataHandler> outboundServiceDataHandler, Device device)
: m_wolkConnectivityService(std::move(wolkConnectivityService))
, m_moduleConnectivityService(std::move(moduleConnectivityService))
, m_persistence(persistence)
, m_inboundWolkaboutMessageHandler(std::move(inboundWolkaboutMessageHandler))
, m_inboundModuleMessageHandler(std::move(inboundModuleMessageHandler))
, m_outboundServiceDataHandler(std::move(outboundServiceDataHandler))
, m_device(device)
, m_actuationHandlerLambda(nullptr)
, m_actuatorStatusProviderLambda(nullptr)
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

void Wolk::handleActuatorSetCommand(const ActuatorSetCommand& command)
{
	if (auto provider = m_actuationHandler.lock())
	{
		provider->handleActuation(command.getReference(), command.getValue());
	}
	else if (m_actuationHandlerLambda)
	{
		m_actuationHandlerLambda(command.getReference(), command.getValue());
	}

	publishActuatorStatus(command.getReference());
}

void Wolk::handleActuatorGetCommand(const ActuatorGetCommand& command)
{
	publishActuatorStatus(command.getReference());
}

//void Wolk::publishFirmwareVersion()
//{
//	if(m_firmwareUpdateService)
//	{
//		const auto firmwareVerion = m_firmwareUpdateService->getFirmwareVersion();
//		const std::shared_ptr<OutboundMessage> outboundMessage =
//				OutboundMessageFactory::makeFromFirmwareVersion(m_device.getDeviceKey(), firmwareVerion);

//		if (!(outboundMessage && m_connectivityService->publish(outboundMessage)))
//		{
//			// TODO Log error
//		}
//	}
//}

void Wolk::connectToWolkabout()
{
	addToCommandBuffer([=]() -> void {
		if(!m_wolkConnectivityService->connect())
		{
			connectToWolkabout();
		}
	});
}

void Wolk::connectToModules()
{
	addToCommandBuffer([=]() -> void {
		if(!m_moduleConnectivityService->connect())
		{
			connectToModules();
		}
	});
}

Wolk::ConnectivityFacade::ConnectivityFacade(InboundMessageHandler* handler, std::function<void()> connectionLostHandler) :
	m_messageHandler{handler}, m_connectionLostHandler{connectionLostHandler}
{
}

void Wolk::ConnectivityFacade::messageReceived(const std::string& topic, const std::string& message)
{
	m_messageHandler->messageReceived(topic, message);
}

void Wolk::ConnectivityFacade::connectionLost()
{
	m_connectionLostHandler();
}

const std::vector<std::string>& Wolk::ConnectivityFacade::getTopics() const
{
	return m_messageHandler->getTopics();
}
}
