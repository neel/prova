cmake_minimum_required(VERSION 3.0)

project(provenance-server)

find_package(Boost REQUIRED)
find_package(nlohmann_json REQUIRED)

add_executable(sequence sequence.cpp)
set_property(TARGET sequence PROPERTY CXX_STANDARD 20)
target_link_libraries(sequence ${BOOST_LIBRARIES} nlohmann_json::nlohmann_json tash)
