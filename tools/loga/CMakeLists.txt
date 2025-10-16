cmake_minimum_required(VERSION 3.6)

project(provenance-alignment)

set(Boost_NO_BOOST_CMAKE        ON  CACHE BOOL "Use FindBoost module, not BoostConfig")
set(Boost_USE_STATIC_LIBS       OFF)
set(Boost_USE_MULTITHREADED     ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

if (MSVC)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()

find_package(Boost REQUIRED COMPONENTS filesystem system serialization process thread program_options)
find_package(nlohmann_json REQUIRED)

find_package(Armadillo REQUIRED)
find_package(mlpack 4.0 REQUIRED)
find_package(cereal REQUIRED)
find_package(igraph REQUIRED)

include_directories(includes)

set(SOURCES
  sources/index.cpp
  sources/face.cpp
  sources/collection.cpp
  sources/alignment.cpp
  sources/multi_alignment.cpp
  sources/segment.cpp
  sources/graph.cpp
  sources/path.cpp
  sources/zone.cpp
  sources/distance.cpp
  sources/cluster.cpp
  sources/group.cpp
  sources/bio_alignment.cpp
  sources/tokenized_collection.cpp
  sources/token.cpp
  sources/tokenized_group.cpp
  sources/tokenized_alignment.cpp
  sources/tokenized_distance.cpp
  sources/tokenized_multi_alignment.cpp
  sources/knn_cluster.cpp
)

set(INCLUDES
  includes/loga/fwd.h
  includes/loga/face.h
  includes/loga/index.h
  includes/loga/collection.h
  includes/loga/alignment.h
  includes/loga/multi_alignment.h
  includes/loga/segment.h
  includes/loga/graph.h
  includes/loga/path.h
  includes/loga/zone.h
  includes/loga/package.h
  includes/loga/distance.h
  includes/loga/cluster.h
  includes/loga/group.h
  includes/loga/bio_alignment.h
  includes/loga/token.h
  includes/loga/tokenized_collection.h
  includes/loga/tokenized_group.h
  includes/loga/tokenized_alignment.h
  includes/loga/tokenized_distance.h
  includes/loga/tokenized_multi_alignment.h
  includes/loga/colors.h
  includes/loga/knn_cluster.h
)

add_library(prova-loga SHARED ${SOURCES} ${INCLUDES})
set_property(TARGET prova-loga PROPERTY CXX_STANDARD 20)
target_link_libraries(prova-loga PUBLIC Boost::headers nlohmann_json::nlohmann_json igraph::igraph)
target_link_libraries(prova-loga PRIVATE Boost::filesystem Boost::system Boost::process Boost::thread Boost::program_options cereal::cereal ${MLPACK_LIBRARIES} ${ARMADILLO_LIBRARIES})
if (MSVC)
  target_compile_definitions(prova-loga PRIVATE NOMINMAX _WINSOCKAPI_ WIN32_LEAN_AND_MEAN)
  target_compile_options(prova-loga PRIVATE /FI${CMAKE_SOURCE_DIR}/tools/includes/prova/win32_winsock_shim.hpp)
  target_compile_definitions(prova-loga PRIVATE $<$<PLATFORM_ID:Windows>:_WINSOCKAPI_> $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)
endif()

add_executable(loga sources/main.cpp)
set_property(TARGET loga PROPERTY CXX_STANDARD 20)
target_link_libraries(loga PRIVATE Boost::serialization Boost::thread Boost::program_options nlohmann_json::nlohmann_json prova-loga ${MLPACK_LIBRARIES} ${ARMADILLO_LIBRARIES})
if (MSVC)
  target_compile_definitions(loga PRIVATE NOMINMAX _WINSOCKAPI_ WIN32_LEAN_AND_MEAN)
  target_compile_options(loga PRIVATE /FI${CMAKE_SOURCE_DIR}/tools/includes/prova/win32_winsock_shim.hpp)
  target_compile_definitions(loga PRIVATE $<$<PLATFORM_ID:Windows>:_WINSOCKAPI_> $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)
endif()

add_executable(tloga sources/tokenized.cpp)
set_property(TARGET tloga PROPERTY CXX_STANDARD 20)
target_link_libraries(tloga PRIVATE Boost::serialization Boost::thread Boost::program_options nlohmann_json::nlohmann_json prova-loga ${MLPACK_LIBRARIES} ${ARMADILLO_LIBRARIES})
if (MSVC)
  target_compile_definitions(tloga PRIVATE NOMINMAX _WINSOCKAPI_ WIN32_LEAN_AND_MEAN)
  target_compile_options(tloga PRIVATE /FI${CMAKE_SOURCE_DIR}/tools/includes/prova/win32_winsock_shim.hpp)
  target_compile_definitions(tloga PRIVATE $<$<PLATFORM_ID:Windows>:_WINSOCKAPI_> $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)
endif()
