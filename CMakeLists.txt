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

# Options of this project
OPTION(BUILD_SHARED "Build this library as a shared library" ON)
option(BUILD_EXECUTABLE "Build the standalone ${PROJECT_NAME} application." ON)
if (NOT ${BUILD_EXECUTABLE})
    message(STATUS "Skipping build of ${PROJECT_NAME} application")
endif ()
option(BUILD_TESTS "Build the library with GTest." ON)

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
set(CMAKE_CXX_STANDARD 11)
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
link_directories(${CMAKE_PREFIX_PATH}/lib)
link_directories(${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
include_directories(SYSTEM ${CMAKE_BINARY_DIR}/include)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Bring in WolkSDK-Cpp
if (NOT TARGET WolkAboutCore)
    set(BUILD_CONNECTIVITY ON CACHE BOOL "Build the library with Paho MQTT and allow MQTT connection to the platform.")
    set(BUILD_POCO OFF CACHE BOOL "Build the library with Poco.")
    set(BUILD_GTEST ${BUILD_TESTS} CACHE BOOL "Build the library with GTest.")
    set(BUILD_AWS_LOG_UPLOADER OFF CACHE BOOL "Build the library with AwsLogUploader.")
    add_subdirectory(WolkSDK-Cpp)
    include_directories(WolkSDK-Cpp)
endif ()

# Define the version of the library
set(WOLKGATEWAY_VERSION_MAJOR 5)
set(WOLKGATEWAY_VERSION_MINOR 0)
set(WOLKGATEWAY_VERSION_PATCH 0)
set(WOLKGATEWAY_VERSION_SUFFIX prerelease)

set(WOLKGATEWAY_VERSION_STRING "${WOLKGATEWAY_VERSION_MAJOR}.${WOLKGATEWAY_VERSION_MINOR}.${WOLKGATEWAY_VERSION_PATCH}")
if (NOT "${WOLKGATEWAY_VERSION_SUFFIX}" STREQUAL "")
    set(WOLKGATEWAY_VERSION_STRING "${WOLKGATEWAY_VERSION_STRING}-${WOLKGATEWAY_VERSION_SUFFIX}")
endif ()

# WolkGateway library
set(LIB_SOURCE_FILES
        wolk/FileHandler.cpp
        wolk/GatewayInboundDeviceMessageHandler.cpp
        wolk/GatewayInboundPlatformMessageHandler.cpp
        wolk/model/GatewayDevice.cpp
        wolk/OutboundRetryMessageHandler.cpp
        wolk/persistence/filesystem/GatewayCircularFileSystemPersistence.cpp
        wolk/persistence/filesystem/GatewayFilesystemPersistence.cpp
        wolk/persistence/filesystem/MessagePersister.cpp
        wolk/persistence/inmemory/GatewayInMemoryPersistence.cpp
        wolk/repository/file/FSFileRepository.cpp
        wolk/repository/existing_device/JsonFileExistingDevicesRepository.cpp
        wolk/repository/device/SQLiteDeviceRepository.cpp
        wolk/service/data/DataService.cpp
        wolk/service/data/ExternalDataService.cpp
        wolk/service/data/GatewayDataService.cpp
        wolk/service/data/InternalDataService.cpp
        wolk/service/DataHandlerApiFacade.cpp
        wolk/service/FileDownloader.cpp
        wolk/service/FileDownloadService.cpp
        wolk/service/FirmwareUpdateService.cpp
        wolk/service/GatewayUpdateService.cpp
        wolk/service/KeepAliveService.cpp
        wolk/service/PublishingService.cpp
        wolk/service/platform_status/PlatformStatusService.cpp
        wolk/service/SubdeviceRegistrationService.cpp
        wolk/StatusMessageRouter.cpp
        wolk/WolkBuilder.cpp
        wolk/Wolk.cpp
        wolk/WolkDefault.cpp
        wolk/WolkExternal.cpp)
set(LIB_HEADER_FILES
        wolk/api/ConnectionStatusListener.h
        wolk/api/DataHandler.h
        wolk/api/DataProvider.h
        wolk/api/FeedUpdateHandler.h
        wolk/api/FileListener.h
        wolk/model/GatewayDevice.h
        wolk/model/SubdeviceManagement.h
        wolk/persistence/filesystem/GatewayCircularFileSystemPersistence.h
        wolk/persistence/filesystem/GatewayFilesystemPersistence.h
        wolk/persistence/filesystem/MessagePersister.h
        wolk/persistence/GatewayPersistence.h
        wolk/persistence/inmemory/GatewayInMemoryPersistence.h
        wolk/repository/device/DeviceRepository.h
        wolk/repository/existing_device/ExistingDevicesRepository.h
        wolk/repository/file/FileRepository.h
        wolk/repository/file/FSFileRepository.h
        wolk/repository/existing_device/JsonFileExistingDevicesRepository.h
        wolk/repository/device/SQLiteDeviceRepository.h
        wolk/service/data/DataService.h
        wolk/service/data/ExternalDataService.h
        wolk/service/data/GatewayDataService.h
        wolk/service/data/InternalDataService.h
        wolk/service/DataHandlerApiFacade.h
        wolk/service/FileDownloader.h
        wolk/service/FileDownloadService.h
        wolk/service/FirmwareUpdateService.h
        wolk/service/GatewayUpdateService.h
        wolk/service/KeepAliveService.h
        wolk/service/PublishingService.h
        wolk/service/SubdeviceRegistrationService.h
        wolk/service/UrlFileDownloader.h
        wolk/service/WolkaboutFileDownloader.h
        wolk/service/platform_status/PlatformStatusService.h
        wolk/FileHandler.h
        wolk/FirmwareInstaller.h
        wolk/GatewayInboundDeviceMessageHandler.h
        wolk/GatewayInboundPlatformMessageHandler.h
        wolk/InboundDeviceMessageHandler.h
        wolk/InboundPlatformMessageHandler.h
        wolk/OutboundMessageHandler.h
        wolk/OutboundRetryMessageHandler.h
        wolk/StatusMessageRouter.h
        wolk/WolkBuilder.h
        wolk/WolkDefault.h
        wolk/WolkExternal.h
        wolk/Wolk.h)

file(COPY wolk/ DESTINATION ${CMAKE_LIBRARY_INCLUDE_DIRECTORY}/wolk/ PATTERN *.cpp EXCLUDE)

add_library(${PROJECT_NAME} ${LIB_TYPE} ${LIB_SOURCE_FILES} ${LIB_HEADER_FILES})
target_link_libraries(${PROJECT_NAME} WolkAboutCore z OpenSSL::Crypto Threads::Threads sqlite3)
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${PROJECT_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_LIBRARY_INCLUDE_DIRECTORY})
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${WOLKGATEWAY_VERSION_STRING} SOVERSION ${WOLKGATEWAY_VERSION_MAJOR}.${WOLKGATEWAY_VERSION_MINOR}.${WOLKGATEWAY_VERSION_PATCH})

# Create the install rule
install(DIRECTORY ${CMAKE_LIBRARY_INCLUDE_DIRECTORY} DESTINATION ${CMAKE_INSTALL_PREFIX}/include PATTERN *.h)
install(DIRECTORY ${CMAKE_PREFIX_PATH}/include DESTINATION ${CMAKE_INSTALL_PREFIX} PATTERN *.h)
install(DIRECTORY ${CMAKE_PREFIX_PATH}/lib/ DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)

# Tests
if (${BUILD_TESTS})
    file(GLOB_RECURSE TESTS_SOURCE_FILES "tests/*.cpp")
    file(GLOB_RECURSE TESTS_HEADER_FILES "tests/*.h")

    add_executable(${PROJECT_NAME}Tests ${TESTS_SOURCE_FILES} ${TESTS_HEADER_FILES})
    target_link_libraries(${PROJECT_NAME}Tests ${PROJECT_NAME} gtest gmock gtest_main gmock_main)
    target_include_directories(${PROJECT_NAME}Tests PUBLIC "tests")
    target_include_directories(${PROJECT_NAME}Tests SYSTEM PRIVATE ${CMAKE_LIBRARY_INCLUDE_DIRECTORY})
    set_target_properties(${PROJECT_NAME}Tests PROPERTIES INSTALL_RPATH "$ORIGIN/lib")
    add_dependencies(${PROJECT_NAME}Tests libgtest)

    add_custom_target(tests ${PROJECT_NAME}Tests)
    add_test(NAME WolkGatewayUnitTests COMMAND ./bin/${PROJECT_NAME}Tests)
endif ()

# WolkGateway executable
if (${BUILD_EXECUTABLE})
    file(GLOB_RECURSE BIN_SOURCE_FILES "application/*.cpp")
    file(GLOB_RECURSE BIN_HEADER_FILES "application/*.h")

    add_executable(${PROJECT_NAME}App ${BIN_SOURCE_FILES} ${BIN_HEADER_FILES})
    target_link_libraries(${PROJECT_NAME}App WolkGateway)
    set_target_properties(${PROJECT_NAME}App PROPERTIES INSTALL_RPATH "$ORIGIN/lib")

    install(TARGETS ${PROJECT_NAME}App DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
    install(FILES out/ca.crt DESTINATION /etc/wolkGateway
            PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ)
    install(FILES out/gatewayConfiguration.json DESTINATION /etc/wolkGateway
            PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ)
endif ()

# Add the format rule
add_custom_target(format
        COMMAND "clang-format" -i -sort-includes -style=file ${LIB_HEADER_FILES} ${LIB_SOURCE_FILES}
        ${BIN_HEADER_FILES} ${BIN_SOURCE_FILES} ${TESTS_HEADER_FILES} ${TESTS_SOURCE_FILES}
        WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
        COMMENT "[Formatting source code]"
        VERBATIM)
