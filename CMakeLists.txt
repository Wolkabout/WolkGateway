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

include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(NO_OUTPUT_DIRS KEEP_RPATHS)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS}    -Wall -Wextra -Weffc++ -pedantic                                           \
                                            -pedantic-errors -Wcast-align                                              \
                                            -Wcast-qual -Wconversion                                                   \
                                            -Wdisabled-optimization                                                    \
                                            -Wfloat-equal -Wformat=2                                                   \
                                            -Wformat-nonliteral -Wformat-security                                      \
                                            -Wformat-y2k                                                               \
                                            -Wimport  -Winit-self                                                      \
                                            -Wmissing-field-initializers -Wmissing-format-attribute                    \
                                            -Wmissing-include-dirs -Wmissing-noreturn                                  \
                                            -Wpointer-arith                                                            \
                                            -Wno-packed  -Wno-padded -Wredundant-decls                                 \
                                            -Wshadow -Wstack-protector                                                 \
                                            -Wstrict-aliasing=2                                                        \
                                            -Wunreachable-code -Wunused                                                \
                                            -Wunused-parameter                                                         \
                                            -Wvariadic-macros                                                          \
                                            -Wwrite-strings")

add_definitions(-DOPENSSL)
add_subdirectory(mqtt)

# WolkGateway
include_directories("mqtt")
include_directories("src")
include_directories(SYSTEM "${CONAN_INCLUDE_DIRS_OPENSSL}")
include_directories(SYSTEM "${CONAN_INCLUDE_DIRS_POCO}")

file(GLOB_RECURSE HEADER_FILES "src/*.h" "src/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} ssl crypto z PocoUtil PocoCrypto PocoData PocoDataSQLite PocoFoundation paho-mqtt pthread)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-Wl,-rpath,./")

# Tests
include_directories("tests")
include_directories(SYSTEM "${CONAN_INCLUDE_DIRS_GTEST}")

file(GLOB_RECURSE TESTS_HEADER_FILES "tests/*.h" "tests/*.hpp")
file(GLOB_RECURSE TESTS_SOURCE_FILES "tests/*.cpp")

add_executable(tests ${TESTS_HEADER_FILES} ${TESTS_SOURCE_FILES})
target_link_libraries(tests ${PROJECT_NAME} gmock_main gmock gtest)
set_target_properties(tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(tests PROPERTIES LINK_FLAGS "-Wl,-rpath,./")

# CMake utilities
add_subdirectory(cmake)

# Example
add_executable(example example/Application.cpp)
target_link_libraries(example ${PROJECT_NAME})
set_target_properties(example PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(example PROPERTIES LINK_FLAGS "-Wl,-rpath,./")
