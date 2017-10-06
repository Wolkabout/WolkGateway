# "Librarize" Paho MQTT
include_directories("./")
include_directories("${OPENSSL_INCLUDE_DIR}")

file(GLOB_RECURSE PAHO_MQTT_SRC "./*.c" "./*.cpp")

add_library(paho-mqtt ${PAHO_MQTT_SRC})
target_link_libraries(paho-mqtt crypto ssl pthread)
