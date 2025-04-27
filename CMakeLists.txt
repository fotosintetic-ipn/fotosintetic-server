cmake_minimum_required(VERSION 3.14)
include(FetchContent)

set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES OFF)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_LIBRARIES OFF)
set(CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS OFF)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_INCLUDES OFF)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_LIBRARIES OFF)
set(CMAKE_C_USE_RESPONSE_FILE_FOR_OBJECTS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
    OXIM-server
    VERSION 0.1.0
    DESCRIPTION "The server responsible for receiving, storing and sending OXIM-user related information."
    HOMEPAGE_URL "https://example.com/"
    LANGUAGES CXX
)

# ---- Declare executable ----

add_executable(OXIM-server_exe source/main.cpp)
add_executable(OXIM-server::exe ALIAS OXIM-server_exe)

set_property(TARGET OXIM-server_exe PROPERTY OUTPUT_NAME OXIM-server)

target_compile_features(OXIM-server_exe PRIVATE cxx_std_17)

# ---- Include libraries ----

include(cmake/CPM.cmake)

set(CROW_BUILD_EXAMPLES OFF)
set(CROW_BUILD_TESTS OFF)

CPMAddPackage("gh:CrowCpp/Crow#v1.2.1.2")

set(BUILD_MARIADB_CONNECTOR ON)
set(BUILD_TESTING OFF)

CPMAddPackage("gh:rbock/sqlpp11#abc9e30b5e6493f9560f2a516c521e34a194ddfc")

CPMAddPackage("gh:nlohmann/json#v3.12.0")

target_link_libraries(OXIM-server_exe
  PRIVATE
  sqlpp11::sqlpp11
  sqlpp11::mariadb
  Crow::Crow
  nlohmann_json::nlohmann_json
)

if(WIN32)
  target_link_libraries(OXIM-server_exe PUBLIC wsock32)
  target_link_libraries(OXIM-server_exe PUBLIC ws2_32)
endif()
