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
#include "gateway/connectivity/GatewayMessageRouter.h"
#undef private
#undef protected

#include "core/utilities/Logger.h"
#include "tests/mocks/GatewaySubdeviceProtocolMock.h"

#include <gtest/gtest.h>

using namespace wolkabout;
using namespace wolkabout::gateway;
using namespace ::testing;

class GatewayMessageRouterTests : public Test
{
public:
    static void SetUpTestCase() { Logger::init(LogLevel::TRACE, Logger::Type::CONSOLE); }

    void SetUp() override
    {
        service = std::unique_ptr<GatewayMessageRouter>{new GatewayMessageRouter{m_gatewaySubdeviceProtocolMock}};
    }

    std::unique_ptr<GatewayMessageRouter> service;

    GatewaySubdeviceProtocolMock m_gatewaySubdeviceProtocolMock;
};
