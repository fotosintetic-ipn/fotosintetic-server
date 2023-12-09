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

set(CROW_BUILD_EXAMPLES OFF)
set(CROW_BUILD_TESTS OFF)

FetchContent_Declare(Crow GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
                          GIT_TAG edf12f699ec3bf6f751cf73cb97f32919e48ca6e)
FetchContent_MakeAvailable(Crow)

set(BUILD_MARIADB_CONNECTOR ON)
set(BUILD_TESTING OFF)

FetchContent_Declare(Sqlpp11 GIT_REPOSITORY https://github.com/rbock/sqlpp11.git
                             GIT_TAG 6cc2067ef5fd91612f31509384e78d36c74c7657) 
FetchContent_MakeAvailable(Sqlpp11)

FetchContent_Declare(json GIT_REPOSITORY https://github.com/nlohmann/json.git
                          GIT_TAG 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03)
FetchContent_MakeAvailable(json)

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
