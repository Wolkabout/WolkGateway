# "Librarize" Paho MQTT
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -w")

include_directories("./")
include_directories(SYSTEM "${CONAN_INCLUDE_DIRS_OPENSSL}")

file(GLOB_RECURSE PAHO_MQTT_SRC "./*.c" "./*.cpp")

add_library(paho-mqtt SHARED ${PAHO_MQTT_SRC})
target_link_libraries(paho-mqtt ${CONAN_LIBS} pthread)
set_target_properties(paho-mqtt PROPERTIES LINK_FLAGS "-Wl,-rpath,./")
