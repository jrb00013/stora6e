cmake_minimum_required(VERSION 3.16)
project(stora6e VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(STORA6E_BUILD_FRONTEND "Build web UI with npm" ON)

include(CTest)

add_subdirectory(backend)

if(STORA6E_BUILD_FRONTEND)
  find_program(NPM_EXECUTABLE npm)
  if(NPM_EXECUTABLE)
    set(FRONTEND_DIR "${CMAKE_SOURCE_DIR}/frontend")
    set(FRONTEND_DIST "${FRONTEND_DIR}/dist")
    add_custom_target(stora6e_frontend
      COMMAND ${NPM_EXECUTABLE} ci
      COMMAND ${NPM_EXECUTABLE} run build
      WORKING_DIRECTORY ${FRONTEND_DIR}
      COMMENT "Building stora6e web UI"
      BYPRODUCTS ${FRONTEND_DIST}/index.html
    )
    add_dependencies(stora6e stora6e_frontend)
    add_custom_command(TARGET stora6e POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${FRONTEND_DIST} $<TARGET_FILE_DIR:stora6e>/web
      COMMENT "Copying web UI to output directory"
    )
  else()
    message(STATUS "npm not found — skipping frontend build")
  endif()
endif()

install(TARGETS stora6e RUNTIME DESTINATION bin)
