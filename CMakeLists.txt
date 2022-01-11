# Copyright 2018 WolkAbout Technology s.r.o.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.5)
project(WolkGateway C CXX)

set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS 1)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}    -Wall -Wextra -pedantic                                                    \
                                            -pedantic-errors -Wcast-align                                              \
                                            -Wcast-qual -Wconversion                                                   \
                                            -Wdisabled-optimization                                                    \
                                            -Wfloat-equal -Wformat=2                                                   \
                                            -Werror=init-self                                                          \
                                            -Werror=missing-field-initializers -Wmissing-format-attribute              \
                                            -Wmissing-include-dirs -Wmissing-noreturn                                  \
                                            -Werror=pointer-arith                                                      \
                                            -Wno-packed  -Wno-padded -Wredundant-decls                                 \
                                            -Werror=shadow -Werror=stack-protector                                     \
                                            -Wstrict-aliasing=2 -Wno-unused                                            \
                                            -Werror=unreachable-code                                                   \
                                            -Wvariadic-macros -Werror=overloaded-virtual                               \
                                            -Wwrite-strings -Werror=non-virtual-dtor -Werror=return-type")


get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if (HAS_PARENT)
    option(BUILD_APP "" OFF)
else ()
    option(BUILD_APP "" ON)
    message(STATUS "Skipping build of ${PROJECT_NAME} app")
endif ()

# library output
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR}/build)
set(CMAKE_LIBRARY_INCLUDE_DIRECTORY ${CMAKE_PREFIX_PATH}/include)
set(POCO_INSTALL_DIR ${CMAKE_BINARY_DIR})
include_directories(SYSTEM ${CMAKE_BINARY_DIR}/include)
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
link_directories(${CMAKE_PREFIX_PATH}/lib)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# WolkAbout c++ SDK
set(BUILD_CONNECTIVITY ON CACHE BOOL "Build the library with Paho MQTT and allow MQTT connection to the platform.")
set(BUILD_POCO OFF CACHE BOOL "Build the library with Poco.")
set(BUILD_GTEST ON CACHE BOOL "Build the library with GTest.")
set(BUILD_AWS_LOG_UPLOADER OFF CACHE BOOL "Build the library with AwsLogUploader.")
option(BUILD_AWS_LOG_UPLOADER "" OFF)
add_subdirectory(WolkSDK-Cpp)

# WolkGateway library
set(LIB_HEADER_FILES
    src/ActuationHandler.h
    src/ActuatorStatusProvider.h
    src/api/DataHandler.h
    src/api/DataProvider.h
    src/api/FeedUpdateHandler.h
    src/ConfigurationHandler.h
    src/ConfigurationProvider.h
    src/ConnectionStatusListener.h
    src/FileHandler.h
    src/FileListener.h
    src/FirmwareInstaller.h
    src/GatewayInboundDeviceMessageHandler.h
    src/GatewayInboundPlatformMessageHandler.h
    src/InboundDeviceMessageHandler.h
    src/InboundPlatformMessageHandler.h
    src/model/GatewayDevice.h
    src/model/SubdeviceManagement.h
    src/OutboundMessageHandler.h
    src/OutboundRetryMessageHandler.h
    src/persistence/filesystem/GatewayCircularFileSystemPersistence.h
    src/persistence/filesystem/GatewayFilesystemPersistence.h
    src/persistence/filesystem/MessagePersister.h
    src/persistence/GatewayPersistence.h
    src/persistence/inmemory/GatewayInMemoryPersistence.h
#    src/protocol/GatewayDataProtocol.h
#    src/protocol/GatewayFirmwareUpdateProtocol.h
    src/protocol/GatewayProtocol.h
#    src/protocol/GatewayStatusProtocol.h
#    src/protocol/GatewaySubdeviceRegistrationProtocol.h
#    src/protocol/json/JsonGatewayDataProtocol.h
#    src/protocol/json/JsonGatewayDFUProtocol.h
#    src/protocol/json/JsonGatewayStatusProtocol.h
#    src/protocol/json/JsonGatewaySubdeviceRegistrationProtocol.h
#    src/RegistrationMessageRouter.h
    src/repository/DeviceRepository.h
    src/repository/ExistingDevicesRepository.h
#    src/repository/FileInfo.h
    src/repository/FileRepository.h
    src/repository/FSFileRepository.h
    src/repository/JsonFileExistingDevicesRepository.h
    src/repository/SQLiteDeviceRepository.h
    src/service/data/DataService.h
    src/service/data/ExternalDataService.h
    src/service/data/GatewayDataService.h
    src/service/data/InternalDataService.h
#    src/service/DataHandlerApiFacade.h
    src/service/FileDownloader.h
    src/service/FileDownloadService.h
    src/service/FirmwareUpdateService.h
    src/service/GatewayUpdateService.h
    src/service/KeepAliveService.h
    src/service/PublishingService.h
#    src/service/status/DeviceStatusService.h
#    src/service/status/ExternalDeviceStatusService.h
#    src/service/status/InternalDeviceStatusService.h
    src/service/status/PlatformStatusService.h
    src/service/SubdeviceRegistrationService.h
    src/service/UrlFileDownloader.h
    src/service/WolkaboutFileDownloader.h
    src/StatusMessageRouter.h
    src/Version.h
    src/WolkBuilder.h
    src/WolkDefault.h
    src/WolkExternal.h
    src/Wolk.h
    )

set(LIB_SOURCE_FILES
    src/FileHandler.cpp
    src/GatewayInboundDeviceMessageHandler.cpp
    src/GatewayInboundPlatformMessageHandler.cpp
    src/model/GatewayDevice.cpp
    src/OutboundRetryMessageHandler.cpp
    src/persistence/filesystem/GatewayCircularFileSystemPersistence.cpp
    src/persistence/filesystem/GatewayFilesystemPersistence.cpp
    src/persistence/filesystem/MessagePersister.cpp
    src/persistence/inmemory/GatewayInMemoryPersistence.cpp
    src/protocol/GatewayProtocol.cpp
#    src/protocol/json/JsonGatewayDataProtocol.cpp
#    src/protocol/json/JsonGatewayDFUProtocol.cpp
#    src/protocol/json/JsonGatewayStatusProtocol.cpp
#    src/protocol/json/JsonGatewaySubdeviceRegistrationProtocol.cpp
#    src/RegistrationMessageRouter.cpp
    src/repository/FSFileRepository.cpp
    src/repository/JsonFileExistingDevicesRepository.cpp
#    src/repository/SQLiteDeviceRepository.cpp
    src/service/data/DataService.cpp
    src/service/data/ExternalDataService.cpp
    src/service/data/GatewayDataService.cpp
    src/service/data/InternalDataService.cpp
#    src/service/DataHandlerApiFacade.cpp
    src/service/FileDownloader.cpp
    src/service/FileDownloadService.cpp
    src/service/FirmwareUpdateService.cpp
    src/service/GatewayUpdateService.cpp
    src/service/KeepAliveService.cpp
    src/service/PublishingService.cpp
#    src/service/status/DeviceStatusService.cpp
#    src/service/status/ExternalDeviceStatusService.cpp
#    src/service/status/InternalDeviceStatusService.cpp
    src/service/status/PlatformStatusService.cpp
    src/service/SubdeviceRegistrationService.cpp
    src/StatusMessageRouter.cpp
    src/WolkBuilder.cpp
    src/Wolk.cpp
    src/WolkDefault.cpp
    src/WolkExternal.cpp
    )

add_library(${PROJECT_NAME} SHARED ${LIB_SOURCE_FILES} ${LIB_HEADER_FILES})
target_link_libraries(${PROJECT_NAME} WolkAboutCore z OpenSSL::Crypto Threads::Threads sqlite3)
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${CMAKE_LIBRARY_INCLUDE_DIRECTORY})
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")


# Tests
file(GLOB_RECURSE TESTS_HEADER_FILES "tests/*.h" "tests/*.hpp")
file(GLOB_RECURSE TESTS_SOURCE_FILES "tests/*.cpp")

add_executable(${PROJECT_NAME}Tests ${TESTS_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME}Tests ${PROJECT_NAME} gtest gmock gtest_main gmock_main)
target_include_directories(${PROJECT_NAME}Tests PUBLIC "tests")
target_include_directories(${PROJECT_NAME}Tests SYSTEM PRIVATE ${CMAKE_LIBRARY_INCLUDE_DIRECTORY})
set_target_properties(${PROJECT_NAME}Tests PROPERTIES INSTALL_RPATH "$ORIGIN/lib")
add_dependencies(${PROJECT_NAME}Tests libgtest)

add_custom_target(tests ${PROJECT_NAME}Tests)
add_test(gtest ${PROJECT_NAME}Tests)


# WolkGateway executable
file(GLOB_RECURSE BIN_HEADER_FILES "application/*.h")
file(GLOB_RECURSE BIN_SOURCE_FILES "application/*.cpp")

if (${BUILD_APP})
    add_executable(${PROJECT_NAME}App ${BIN_SOURCE_FILES})
    target_link_libraries(${PROJECT_NAME}App WolkGateway)
    set_target_properties(${PROJECT_NAME}App PROPERTIES INSTALL_RPATH "$ORIGIN/lib")

    # Install target
    install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION /usr
            FILES_MATCHING
            PATTERN "*so*")

    # Place the executable file in /usr/bin
    install(TARGETS ${PROJECT_NAME}App DESTINATION /usr/bin)

    # Place the ca.crt file in /usr/bin
    install(FILES out/ca.crt DESTINATION /etc/wolkGateway
            PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ)

    # Place the config files and service unit file.
    install(FILES out/gatewayConfiguration.json DESTINATION /etc/wolkGateway
            PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ)

    install(CODE "MESSAGE(\"Installed WolkGateway. Configuration files for the module are placed in /etc/wolkGateway.\")")
endif ()

# CMake utilities
add_subdirectory(cmake)
