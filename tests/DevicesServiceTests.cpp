/**
 * Copyright 2022 Wolkabout Technology s.r.o.
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

#include <any>
#include <sstream>

#define private public
#define protected public
#include "gateway/service/devices/DevicesService.h"
#undef private
#undef protected

#include "core/utilities/Logger.h"
#include "tests/mocks/DeviceRepositoryMock.h"
#include "tests/mocks/GatewayRegistrationProtocolMock.h"
#include "tests/mocks/RegistrationProtocolMock.h"
#include "tests/mocks/OutboundMessageHandlerMock.h"
#include "tests/mocks/OutboundRetryMessageHandlerMock.h"

#include <gtest/gtest.h>

using namespace wolkabout;
using namespace wolkabout::gateway;
using namespace ::testing;

class DevicesServiceTests : public Test
{
public:
    static void SetUpTestCase() { Logger::init(LogLevel::TRACE, Logger::Type::CONSOLE); }

    void SetUp() override
    {
        m_gatewayRegistrationProtocolMock = std::make_shared<NiceMock<GatewayRegistrationProtocolMock>>();
        m_localOutboundMessageHandlerMock = std::make_shared<NiceMock<OutboundMessageHandlerMock>>();
        m_deviceRepositoryMock = std::make_shared<NiceMock<DeviceRepositoryMock>>();
        service = std::unique_ptr<DevicesService>{
          new DevicesService{GATEWAY_KEY, m_registrationProtocol, m_platformOutboundMessageHandlerMock,
                             m_platformOutboundRetryMessageHandlerMock, m_gatewayRegistrationProtocolMock,
                             m_localOutboundMessageHandlerMock, m_deviceRepositoryMock}};
    }

    std::unique_ptr<DevicesService> service;

    const std::string GATEWAY_KEY = "TEST_GATEWAY";

    NiceMock<RegistrationProtocolMock> m_registrationProtocol;

    NiceMock<OutboundMessageHandlerMock> m_platformOutboundMessageHandlerMock;

    NiceMock<OutboundRetryMessageHandlerMock> m_platformOutboundRetryMessageHandlerMock;

    std::shared_ptr<GatewayRegistrationProtocolMock> m_gatewayRegistrationProtocolMock;

    std::shared_ptr<OutboundMessageHandlerMock> m_localOutboundMessageHandlerMock;

    std::shared_ptr<DeviceRepositoryMock> m_deviceRepositoryMock;
};
