cmake_minimum_required(VERSION 3.0)

project(provenance-server)

find_package(ZeroMQ REQUIRED)
find_package(Boost REQUIRED)
find_package(Auparse REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(includes)

set(SERVER_SOURCES
    sysaudit/log.cpp
    sysaudit/parser.cpp
    sysaudit/reporter.cpp
    server.cpp
    main.cpp
)

add_executable(server ${SERVER_SOURCES})
set_property(TARGET server PROPERTY CXX_STANDARD 20)

target_link_libraries(server zmq ${BOOST_LIBRARIES} azmq ${AUPARSE_LIBRARIES} nlohmann_json::nlohmann_json tash)
