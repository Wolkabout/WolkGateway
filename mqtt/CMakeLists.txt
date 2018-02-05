# "Librarize" Paho MQTT
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

include_directories("./")
include_directories(SYSTEM "${OPENSSL_INCLUDE_DIR}")

file(GLOB_RECURSE PAHO_MQTT_SRC "./*.c" "./*.cpp")

link_directories(${PROJECT_SOURCE_DIR}/out/lib)

add_library(paho-mqtt SHARED ${PAHO_MQTT_SRC})
target_link_libraries(paho-mqtt crypto ssl pthread)
set_target_properties(paho-mqtt PROPERTIES LINK_FLAGS "-Wl,-rpath,./")
