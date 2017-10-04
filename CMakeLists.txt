# Copyright 2017 WolkAbout Technology s.r.o.
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

cmake_minimum_required(VERSION 3.6)
project(WolkAboutConnector CXX)

set(CMAKE_CXX_STANDARD 11)

include_directories("src")
file(GLOB_RECURSE HEADER_FILES "src/*.h" "src/*.hpp")
file(GLOB_RECURSE SOURCE_FILES "src/*.cpp")

link_directories(${PROJECT_SOURCE_DIR}/out)

set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "./")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

add_library(${PROJECT_NAME} SHARED ${HEADER_FILES} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} paho-mqtt3as paho-mqttpp3 crypto ssl pthread)

add_subdirectory(cmake)

# Example
add_executable(example example/Application.cpp)
target_link_libraries(example ${PROJECT_NAME})

