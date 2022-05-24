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

find_package(Threads REQUIRED)

# WolkAbout c++ SDK
set(BUILD_CONNECTIVITY ON CACHE BOOL "Build the library with Paho MQTT and allow MQTT connection to the platform.")
set(BUILD_POCO ON CACHE BOOL "Build the library with Poco.")
set(BUILD_GTEST ON CACHE BOOL "Build the library with GTest.")
set(BUILD_AWS_LOG_UPLOADER OFF CACHE BOOL "Build the library with AwsLogUploader.")
option(POCO_BUILD_DATA "" ON)
option(POCO_BUILD_NET "" ON)
option(BUILD_AWS_LOG_UPLOADER "" OFF)
add_subdirectory(WolkSDK-Cpp)

# WolkGateway library
file(GLOB_RECURSE LIB_HEADER_FILES "src/*.h" "src/*.hpp")
file(GLOB_RECURSE LIB_SOURCE_FILES "src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${LIB_SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} WolkAboutCore z PocoUtil PocoCrypto PocoData PocoDataSQLite PocoFoundation Threads::Threads)
target_include_directories(${PROJECT_NAME} PUBLIC "src")
target_include_directories(${PROJECT_NAME} SYSTEM PRIVATE ${CMAKE_LIBRARY_INCLUDE_DIRECTORY})
set_target_properties(${PROJECT_NAME} PROPERTIES INSTALL_RPATH "$ORIGIN")
add_dependencies(${PROJECT_NAME} libpoco)


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

    # Place the config files and service unit file.
    install(FILES out/gatewayConfiguration.json DESTINATION /etc/wolkGateway
            PERMISSIONS OWNER_WRITE OWNER_READ GROUP_WRITE GROUP_READ WORLD_READ)

    install(CODE "MESSAGE(\"Installed WolkGateway. Configuration files for the module are placed in /etc/wolkGateway.\")")
endif ()

# CMake utilities
add_subdirectory(cmake)
