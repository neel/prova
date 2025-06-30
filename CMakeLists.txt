cmake_minimum_required(VERSION 3.6)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(WIN32 OR APPLE)
    message(STATUS "HOST = ${CMAKE_HOST_SYSTEM_NAME} HostTARGET = ${CMAKE_SYSTEM_NAME}  WIN32 = '${WIN32}'  APPLE = '${APPLE}'")
    include(cmake/SetupVcpkg.cmake)
endif()


project(provenance)

# add_subdirectory(deps/azmq)
add_subdirectory(deps/tash)
add_subdirectory(deps/QJsonModel)
add_subdirectory(deps/noch)
option(BUILD_SHARED_LIBS "" ON)
add_subdirectory(deps/spoa)

# add_subdirectory(deps/seqan)
# add_subdirectory(server)

# find_path(MLPACK_INCLUDE_DIRS "mlpack.hpp")
# target_include_directories(main PRIVATE ${MLPACK_INCLUDE_DIRS})


# find_package(ZeroMQ REQUIRED)

# set(AGENT_SOURCES
#     agent.c
# )

# add_executable(agent-audit ${AGENT_SOURCES})

# target_link_libraries(agent-audit zmq)
# set_property(TARGET agent-audit PROPERTY C_STANDARD 11)

add_subdirectory(tools)

# install(TARGETS agent-audit RUNTIME DESTINATION bin)


