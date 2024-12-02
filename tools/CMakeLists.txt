cmake_minimum_required(VERSION 3.0)

project(provenance-server)

find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

include_directories(includes)

set(SEQUENCE_SOURCES
	sources/action.cpp
	sources/artifact.cpp
	sources/process.cpp
	sources/session.cpp
	sources/store.cpp
	sources/execution_unit.cpp
)

add_library(prova-tools ${SEQUENCE_SOURCES})
set_property(TARGET prova-tools PROPERTY CXX_STANDARD 20)
target_link_libraries(prova-tools ${BOOST_LIBRARIES} nlohmann_json::nlohmann_json tash)

add_executable(sequence ${SEQUENCE_SOURCES} sequence.cpp)
set_property(TARGET sequence PROPERTY CXX_STANDARD 20)
target_link_libraries(sequence ${BOOST_LIBRARIES} nlohmann_json::nlohmann_json tash)

add_subdirectory(ui)
