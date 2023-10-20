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

# Options of this project
OPTION(BUILD_SHARED "Build this library as a shared library" ON)
option(BUILD_EXECUTABLE "Build the standalone ${PROJECT_NAME} application." ON)
if (NOT ${BUILD_EXECUTABLE})
    message(STATUS "Skipping build of ${PROJECT_NAME} application")
endif ()
option(BUILD_TESTS "Build the library with GTest." ON)
option(BUILD_APT_SYSTEMD_FIRMWARE_UPDATER "Build the optional apt/systemd firmware updaters" ON)

# Configure the paths for output
if (NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
endif ()

if (NOT DEFINED CMAKE_LIBRARY_INCLUDE_DIRECTORY)
    set(CMAKE_LIBRARY_INCLUDE_DIRECTORY "${CMAKE_BINARY_DIR}/include")
endif ()

if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif ()

if (NOT DEFINED CMAKE_PREFIX_PATH)
    set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_BINARY_DIR}/build")
endif ()

# Setting the compiler flags
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}     -Wall -Wextra -pedantic                                                    \
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

if (${BUILD_TESTS})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage -g")
endif ()

if (${BUILD_SHARED})
    set(LIB_TYPE SHARED)
else ()
    set(LIB_TYPE STATIC)
endif ()

# Add linking directories and the include directories
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY} ${CMAKE_PREFIX_PATH}/lib)
include_directories(SYSTEM ${CMAKE_PREFIX_PATH}/include)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Bring in WolkSDK-Cpp
if (NOT TARGET WolkAboutCore)
    set(BUILD_CONNECTIVITY ON CACHE BOOL "Build the library with Paho MQTT and allow MQTT connection to the platform.")
    set(BUILD_EXAMPLES OFF CACHE BOOL "Build the examples/runtimes for testing")
    set(BUILD_POCO OFF CACHE BOOL "Build the library with Poco.")
    set(BUILD_GTEST ${BUILD_TESTS} CACHE BOOL "Build the library with GTest.")
    set(BUILD_AWS_LOG_UPLOADER OFF CACHE BOOL "Build the library with AwsLogUploader.")
    set(BUILD_POCO_HTTP_DOWNLOADER OFF CACHE BOOL "Build the optional `FileDownloader` implementation that uses Poco")
    add_subdirectory(WolkConnect-Cpp)
    include_directories(WolkConnect-Cpp)
endif ()

# Define the version of the library
set(WOLKGATEWAY_VERSION_MAJOR 5)
set(WOLKGATEWAY_VERSION_MINOR 1)
set(WOLKGATEWAY_VERSION_PATCH 4)
set(WOLKGATEWAY_VERSION_SUFFIX)

set(WOLKGATEWAY_VERSION_STRING "${WOLKGATEWAY_VERSION_MAJOR}.${WOLKGATEWAY_VERSION_MINOR}.${WOLKGATEWAY_VERSION_PATCH}")
if (NOT "${WOLKGATEWAY_VERSION_SUFFIX}" STREQUAL "")
    set(WOLKGATEWAY_VERSION_STRING "${WOLKGATEWAY_VERSION_STRING}-${WOLKGATEWAY_VERSION_SUFFIX}")
endif ()

# WolkGateway library
set(LIB_SOURCE_FILES gateway/connectivity/GatewayMessageRouter.cpp
        gateway/repository/DeviceOwnership.cpp
        gateway/repository/existing_device/JsonFileExistingDevicesRepository.cpp
        gateway/repository/device/InMemoryDeviceRepository.cpp
        gateway/repository/device/SQLiteDeviceRepository.cpp
        gateway/service/external_data/ExternalDataService.cpp
        gateway/service/internal_data/InternalDataService.cpp
        gateway/service/platform_status/GatewayPlatformStatusService.cpp
        gateway/service/devices/DevicesService.cpp
        gateway/WolkGatewayBuilder.cpp
        gateway/WolkGateway.cpp)
set(LIB_HEADER_FILES gateway/api/DataHandler.h
        gateway/api/DataProvider.h
        gateway/connectivity/GatewayMessageRouter.h
        gateway/repository/DeviceFilter.h
        gateway/repository/DeviceOwnership.h
        gateway/repository/device/DeviceRepository.h
        gateway/repository/existing_device/ExistingDevicesRepository.h
        gateway/repository/existing_device/JsonFileExistingDevicesRepository.h
        gateway/repository/device/InMemoryDeviceRepository.h
        gateway/repository/device/SQLiteDeviceRepository.h
        gateway/service/external_data/ExternalDataService.h
        gateway/service/internal_data/InternalDataService.h
        gateway/service/devices/DevicesService.h
        gateway/service/platform_status/GatewayPlatformStatusService.h
        gateway/GatewayMessageListener.h
        gateway/WolkGatewayBuilder.h
        gateway/WolkGateway.h)

file(COPY gateway/ DESTINATION ${CMAKE_LIBRARY_INCLUDE_DIRECTORY}/gateway/ PATTERN *.cpp EXCLUDE)

add_library(${PROJECT_NAME} ${LIB_TYPE} ${LIB_SOURCE_FILES} ${LIB_HEADER_FILES})
target_link_libraries(${PROJECT_NAME} WolkAboutConnector z OpenSSL::Crypto Threads::Threads sqlite3)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_LIBRARY_INCLUDE_DIRECTORY})
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${WOLKGATEWAY_VERSION_STRING} SOVERSION ${WOLKGATEWAY_VERSION_MAJOR}.${WOLKGATEWAY_VERSION_MINOR}.${WOLKGATEWAY_VERSION_PATCH})

# Create the install rule
include(GNUInstallDirs)
install(DIRECTORY ${CMAKE_LIBRARY_INCLUDE_DIRECTORY} DESTINATION ${CMAKE_INSTALL_PREFIX} PATTERN *.h)
install(DIRECTORY ${CMAKE_PREFIX_PATH}/include DESTINATION ${CMAKE_INSTALL_PREFIX} PATTERN *.h)
install(DIRECTORY ${CMAKE_PREFIX_PATH}/lib/ DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

# Tests
if (${BUILD_TESTS})
    set(TESTS_SOURCE_FILES tests/DevicesServiceTests.cpp
            tests/ExternalDataServiceTests.cpp
            tests/GatewayMessageRouterTests.cpp
            tests/GatewayPlatformStatusServiceTests.cpp
            tests/InternalDataServiceTests.cpp
            tests/WolkGatewayBuilderTests.cpp
            tests/WolkGatewayTests.cpp)
    set(TESTS_HEADER_FILES tests/mocks/DataHandlerMock.h
            tests/mocks/DataProviderMock.h
            tests/mocks/DeviceRepositoryMock.h
            tests/mocks/DevicesServiceMock.h
            tests/mocks/ExistingDeviceRepositoryMock.h
            tests/mocks/GatewayMessageListenerMock.h
            tests/mocks/GatewayPlatformStatusServiceMock.h)

    enable_testing()
    add_executable(${PROJECT_NAME}Tests ${TESTS_SOURCE_FILES} ${TESTS_HEADER_FILES})
    target_link_libraries(${PROJECT_NAME}Tests ${PROJECT_NAME} gtest gmock gtest_main gmock_main)
    target_include_directories(${PROJECT_NAME}Tests PRIVATE ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/WolkConnect-Cpp ${PROJECT_SOURCE_DIR}/WolkConnect-Cpp/WolkSDK-Cpp)
    target_include_directories(${PROJECT_NAME}Tests PUBLIC ${CMAKE_LIBRARY_INCLUDE_DIRECTORY})
    set_target_properties(${PROJECT_NAME}Tests PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")

    add_test(NAME "WolkGateway_Tests" COMMAND ${PROJECT_NAME}Tests)
endif ()

# WolkGateway executable
if (${BUILD_EXECUTABLE})
    set(BIN_SOURCE_FILES application/Application.cpp
            application/Configuration.cpp)
    set(BIN_HEADER_FILES application/Configuration.h)

    add_executable(${PROJECT_NAME}App ${BIN_SOURCE_FILES} ${BIN_HEADER_FILES})
    target_link_libraries(${PROJECT_NAME}App WolkGateway)
    set_target_properties(${PROJECT_NAME}App PROPERTIES INSTALL_RPATH "$ORIGIN/lib")

    # Install target
    install(DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY} DESTINATION ${CMAKE_INSTALL_PREFIX}
            FILES_MATCHING
            PATTERN "*so*")

    # Place the executable file in /usr/bin
    install(TARGETS ${PROJECT_NAME}App DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)

    # Place the config files and service unit file.
    install(FILES out/gatewayConfiguration.json DESTINATION /etc/wolkGateway
            PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ)
endif ()
