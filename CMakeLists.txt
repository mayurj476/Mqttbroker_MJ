cmake_minimum_required(VERSION 3.10)
project(mqtt_broker)
include(FetchContent)

FetchContent_Declare(
  json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)

FetchContent_MakeAvailable(json)

set(CMAKE_CXX_STANDARD 20)

include_directories(include)


add_executable(mqtt_broker
    src/main.cpp
    src/mqtt_broker.cpp
    src/json_parser.cpp
)

configure_file(${CMAKE_SOURCE_DIR}/config/config.json ${CMAKE_BINARY_DIR}/config.json COPYONLY)
target_link_libraries(mqtt_broker PRIVATE nlohmann_json::nlohmann_json 
                      pthread  
                      )