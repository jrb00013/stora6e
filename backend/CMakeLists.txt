cmake_minimum_required(VERSION 3.16)

include(FetchContent)

FetchContent_Declare(
  httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(httplib)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(json)

add_executable(stora6e
  src/main.cpp
  src/types.cpp
  src/scanner.cpp
  src/categorizer.cpp
  src/scan_manager.cpp
  src/api_server.cpp
  src/util.cpp
)

target_include_directories(stora6e PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${httplib_SOURCE_DIR}
)

target_link_libraries(stora6e PRIVATE nlohmann_json::nlohmann_json pthread)

if(UNIX AND NOT APPLE)
  target_link_libraries(stora6e PRIVATE stdc++fs)
endif()

target_compile_options(stora6e PRIVATE -Wall -Wextra -Wpedantic)
