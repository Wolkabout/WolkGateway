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
#include "gateway/WolkGateway.h"
#include "gateway/WolkGatewayBuilder.h"
#undef private
#undef protected

#include "core/utility/Logger.h"
#include "tests/mocks/DataProtocolMock.h"
#include "tests/mocks/DataProviderMock.h"
#include "tests/mocks/ErrorProtocolMock.h"
#include "tests/mocks/ExistingDeviceRepositoryMock.h"
#include "tests/mocks/FeedUpdateHandlerMock.h"
#include "tests/mocks/FileDownloaderMock.h"
#include "tests/mocks/FileListenerMock.h"
#include "tests/mocks/FirmwareInstallerMock.h"
#include "tests/mocks/FirmwareParametersListenerMock.h"
#include "tests/mocks/GatewayPlatformStatusProtocolMock.h"
#include "tests/mocks/GatewayRegistrationProtocolMock.h"
#include "tests/mocks/MessagePersistenceMock.h"
#include "tests/mocks/ParameterHandlerMock.h"
#include "tests/mocks/PersistenceMock.h"
#include "tests/mocks/RegistrationProtocolMock.h"

#include <gtest/gtest.h>

using namespace wolkabout;
using namespace wolkabout::gateway;
using namespace ::testing;

class WolkGatewayBuilderTests : public Test
{
public:
    static void SetUpTestCase() { Logger::init(LogLevel::TRACE, Logger::Type::CONSOLE); }

    void SetUp() override
    {
        feedUpdateHandlerMock = std::make_shared<NiceMock<FeedUpdateHandlerMock>>();
        parameterHandlerMock = std::make_shared<NiceMock<ParameterHandlerMock>>();
        persistenceMock = std::unique_ptr<PersistenceMock>{new NiceMock<PersistenceMock>};
        messagePersistenceMock = std::unique_ptr<MessagePersistenceMock>{new NiceMock<MessagePersistenceMock>};
        existingDevicesRepositoryMock =
          std::unique_ptr<ExistingDevicesRepositoryMock>{new NiceMock<ExistingDevicesRepositoryMock>};
        dataProtocolMock = std::unique_ptr<DataProtocolMock>{new DataProtocolMock};
        errorProtocolMock = std::unique_ptr<ErrorProtocolMock>{new ErrorProtocolMock};
        fileDownloaderMock = std::unique_ptr<FileDownloaderMock>{new FileDownloaderMock};
        fileListenerMock = std::make_shared<NiceMock<FileListenerMock>>();
        firmwareInstallerMock = std::unique_ptr<FirmwareInstallerMock>{new NiceMock<FirmwareInstallerMock>};
        firmwareParameterListenerMock =
          std::unique_ptr<FirmwareParametersListenerMock>{new NiceMock<FirmwareParametersListenerMock>};
        dataProviderMock = std::unique_ptr<DataProviderMock>{new NiceMock<DataProviderMock>};
    }

    const Device gateway{"TestGateway", "TestPassword", OutboundDataMode::PUSH};

    const std::string platformHost = "platformHost!";

    const std::string platformCaCrt = "platformCaCrt!";

    const std::string localHost = "localHost!";

    std::shared_ptr<FeedUpdateHandlerMock> feedUpdateHandlerMock;

    std::shared_ptr<ParameterHandlerMock> parameterHandlerMock;

    std::unique_ptr<PersistenceMock> persistenceMock;

    std::unique_ptr<MessagePersistenceMock> messagePersistenceMock;

    std::unique_ptr<ExistingDevicesRepositoryMock> existingDevicesRepositoryMock;

    std::unique_ptr<DataProtocolMock> dataProtocolMock;

    const std::chrono::seconds errorRetainTime{10};

    std::unique_ptr<ErrorProtocolMock> errorProtocolMock;

    const std::string fileDownloadLocation = "./files";

    const std::uint64_t maxPacketSize = 1024;

    std::unique_ptr<FileDownloaderMock> fileDownloaderMock;

    std::shared_ptr<FileListenerMock> fileListenerMock;

    std::unique_ptr<FirmwareInstallerMock> firmwareInstallerMock;

    std::unique_ptr<FirmwareParametersListenerMock> firmwareParameterListenerMock;

    const std::uint16_t keepAlive = 10;

    std::unique_ptr<DataProviderMock> dataProviderMock;
};

TEST_F(WolkGatewayBuilderTests, EmptyDeviceKey)
{
    ASSERT_THROW(([] { WolkGatewayBuilder{{"", "", OutboundDataMode::PUSH}}.build(); }()), std::logic_error);
}

TEST_F(WolkGatewayBuilderTests, FirmwareParameterListener)
{
    auto wolk = std::unique_ptr<WolkGateway>{};
    ASSERT_NO_FATAL_FAILURE([&] {
        wolk = WolkGatewayBuilder{gateway}
                 .withFirmwareUpdate(std::move(firmwareParameterListenerMock), fileDownloadLocation)
                 .build();
    }());
    ASSERT_NE(wolk, nullptr);
}

TEST_F(WolkGatewayBuilderTests, FullExample)
{
    auto wolk = std::unique_ptr<WolkGateway>{};
    ASSERT_NO_FATAL_FAILURE([&] {
        wolk = WolkGatewayBuilder{gateway}
                 .platformHost(platformHost)
                 .platformTrustStore(platformCaCrt)
                 .feedUpdateHandler([](const std::string&, const std::map<std::uint64_t, std::vector<Reading>>&) {})
                 .feedUpdateHandler(feedUpdateHandlerMock)
                 .parameterHandler([](const std::string&, const std::vector<Parameter>&) {})
                 .parameterHandler(parameterHandlerMock)
                 .withPersistence(std::move(persistenceMock))
                 .withMessagePersistence(std::move(messagePersistenceMock))
                 .deviceStoragePolicy(DeviceStoragePolicy::FULL)
                 .withExistingDeviceRepository(std::move(existingDevicesRepositoryMock))
                 .withDataProtocol(std::move(dataProtocolMock))
                 .withErrorProtocol(errorRetainTime, std::move(errorProtocolMock))
                 .withFileTransfer(fileDownloadLocation, maxPacketSize)
                 .withFileURLDownload(fileDownloadLocation, std::move(fileDownloaderMock), true, maxPacketSize)
                 .withFileListener(fileListenerMock)
                 .withFirmwareUpdate(std::move(firmwareInstallerMock), fileDownloadLocation)
                 .setMqttKeepAlive(keepAlive)
                 .withInternalDataService(localHost)
                 .withPlatformRegistration()
                 .withLocalRegistration()
                 .withExternalDataService(dataProviderMock.get())
                 .withPlatformStatusService()
                 .build();
    }());
    ASSERT_NE(wolk, nullptr);

    // Call some methods
    ASSERT_NO_FATAL_FAILURE(wolk->m_connectivityService->m_onConnectionLost());
    ASSERT_NO_FATAL_FAILURE(wolk->m_dataService->m_feedUpdateHandler("", {}));
    ASSERT_NO_FATAL_FAILURE(wolk->m_dataService->m_parameterSyncHandler("", {}));
    ASSERT_NO_FATAL_FAILURE(wolk->m_dataService->m_detailsSyncHandler("", {"F1"}, {"A1"}));
}
