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

find_package(Armadillo)
find_package(mlpack 4.0)

include_directories(includes)

set(SOURCES
  sources/index.cpp
  sources/align.cpp
  sources/face.cpp
  sources/collection.cpp
  sources/alignment.cpp
  sources/multi_alignment.cpp
  sources/segment.cpp
  sources/graph.cpp
  sources/path.cpp
  sources/zone.cpp
)

set(INCLUDES
  includes/prova/fwd.h
  includes/prova/face.h
  includes/prova/index.h
  includes/prova/collection.h
  includes/prova/alignment.h
  includes/prova/multi_alignment.h
  includes/prova/segment.h
  includes/prova/graph.h
  includes/prova/path.h
  includes/prova/zone.h
)

if(ARMADILLO_FOUND AND MLPACK_FOUND)

add_library(prova-align SHARED ${SOURCES})
set_property(TARGET prova-align PROPERTY CXX_STANDARD 20)
target_link_libraries(prova-align PUBLIC Boost::headers nlohmann_json::nlohmann_json)
target_link_libraries(prova-align PRIVATE Boost::filesystem Boost::system Boost::process Boost::thread Boost::program_options ${MLPACK_LIBRARIES} ${ARMADILLO_LIBRARIES})
if (MSVC)
  target_compile_definitions(prova-align PRIVATE NOMINMAX _WINSOCKAPI_ WIN32_LEAN_AND_MEAN)
  target_compile_options(prova-align PRIVATE /FI${CMAKE_CURRENT_SOURCE_DIR}/includes/prova/win32_winsock_shim.hpp)
  target_compile_definitions(prova-align PRIVATE $<$<PLATFORM_ID:Windows>:_WINSOCKAPI_> $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)
endif()

add_executable(plogal sources/align.cpp)
set_property(TARGET plogal PROPERTY CXX_STANDARD 20)
target_link_libraries(plogal PRIVATE Boost::serialization Boost::thread Boost::program_options nlohmann_json::nlohmann_json prova-align ${MLPACK_LIBRARIES} ${ARMADILLO_LIBRARIES})
if (MSVC)
  target_compile_definitions(plogal PRIVATE NOMINMAX _WINSOCKAPI_ WIN32_LEAN_AND_MEAN)
  target_compile_options(plogal PRIVATE /FI${CMAKE_CURRENT_SOURCE_DIR}/includes/prova/win32_winsock_shim.hpp)
  target_compile_definitions(plogal PRIVATE $<$<PLATFORM_ID:Windows>:_WINSOCKAPI_> $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)
endif()

endif()
