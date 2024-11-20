cmake_minimum_required(VERSION 3.0)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(provenance)

add_subdirectory(deps/azmq)
add_subdirectory(deps/tash)
add_subdirectory(server)

find_package(ZeroMQ REQUIRED)

set(AGENT_SOURCES
    agent.c
)

add_executable(agent-audit ${AGENT_SOURCES})

target_link_libraries(agent-audit zmq)
set_property(TARGET agent-audit PROPERTY C_STANDARD 11)

install(TARGETS agent-audit RUNTIME DESTINATION bin)
