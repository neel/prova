cmake_minimum_required(VERSION 3.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(provenance)

add_subdirectory(deps/azmq)
add_subdirectory(deps/tash)
add_subdirectory(deps/QJsonModel)
add_subdirectory(server)

# find_path(MLPACK_INCLUDE_DIRS "mlpack.hpp")
# target_include_directories(main PRIVATE ${MLPACK_INCLUDE_DIRS})


find_package(ZeroMQ REQUIRED)

set(AGENT_SOURCES
    agent.c
)

add_executable(agent-audit ${AGENT_SOURCES})

target_link_libraries(agent-audit zmq)
set_property(TARGET agent-audit PROPERTY C_STANDARD 11)

add_subdirectory(tools)

install(TARGETS agent-audit RUNTIME DESTINATION bin)
