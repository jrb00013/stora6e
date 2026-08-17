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

option(STORA6E_BUILD_TESTS "Build the backend unit test suite" ON)

if(STORA6E_BUILD_TESTS)
  include(CTest)

  FetchContent_Declare(
    catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(catch2)
  list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)

  add_library(stora6e_core STATIC
    src/types.cpp
    src/scanner.cpp
    src/categorizer.cpp
    src/scan_manager.cpp
    src/util.cpp
  )
  target_include_directories(stora6e_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
  )
  target_link_libraries(stora6e_core PUBLIC nlohmann_json::nlohmann_json pthread)
  if(UNIX AND NOT APPLE)
    target_link_libraries(stora6e_core PUBLIC stdc++fs)
  endif()

  add_executable(stora6e_tests
    tests/test_categorizer.cpp
  )
  target_link_libraries(stora6e_tests PRIVATE stora6e_core Catch2::Catch2WithMain)

  include(Catch)
  catch_discover_tests(stora6e_tests)
endif()
