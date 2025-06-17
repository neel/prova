cmake_minimum_required(VERSION 3.6)

project(provenance-server)

find_package(Boost REQUIRED COMPONENTS filesystem system process thread multi_index)
set(Boost_USE_STATIC_LIBS        OFF)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

find_package(nlohmann_json REQUIRED)

include_directories(includes)

set(SEQUENCE_SOURCES
	sources/action.cpp
	sources/artifact.cpp
	sources/process.cpp
	sources/session.cpp
	sources/store.cpp
	# sources/string_indexer.cpp
	sources/execution_unit.cpp
	sources/distance.cpp
)

add_library(prova-tools SHARED ${SEQUENCE_SOURCES})
set_property(TARGET prova-tools PROPERTY CXX_STANDARD 20)
target_link_libraries(prova-tools PUBLIC Boost::multi_index nlohmann_json::nlohmann_json)
target_link_libraries(prova-tools PRIVATE Boost::filesystem Boost::system Boost::process Boost::multi_index Boost::thread nlohmann_json::nlohmann_json tash noch-lib)
if (MSVC)
  target_compile_definitions(prova-tools PRIVATE NOMINMAX _WINSOCKAPI_ WIN32_LEAN_AND_MEAN)
  target_compile_options(prova-tools PRIVATE /FI${CMAKE_CURRENT_SOURCE_DIR}/includes/prova/win32_winsock_shim.hpp)
  target_compile_definitions(prova-tools PRIVATE $<$<PLATFORM_ID:Windows>:_WINSOCKAPI_> $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)
endif()

add_executable(sequence ${SEQUENCE_SOURCES} sequence.cpp)
set_property(TARGET sequence PROPERTY CXX_STANDARD 20)
target_link_libraries(sequence PRIVATE Boost::filesystem Boost::system Boost::process Boost::thread nlohmann_json::nlohmann_json tash noch-lib)
if (MSVC)
  target_compile_definitions(sequence PRIVATE NOMINMAX _WINSOCKAPI_ WIN32_LEAN_AND_MEAN)
  target_compile_options(sequence PRIVATE /FI${CMAKE_CURRENT_SOURCE_DIR}/includes/prova/win32_winsock_shim.hpp)
  target_compile_definitions(sequence PRIVATE $<$<PLATFORM_ID:Windows>:_WINSOCKAPI_> $<$<PLATFORM_ID:Windows>:WIN32_LEAN_AND_MEAN>)
endif()

install(TARGETS prova-tools
  EXPORT    ProvaToolsTargets
  RUNTIME   DESTINATION ${CMAKE_INSTALL_BINDINDIR}   # on Windows
  LIBRARY   DESTINATION ${CMAKE_INSTALL_LIBDIR}      # on Linux
  ARCHIVE   DESTINATION ${CMAKE_INSTALL_LIBDIR}      # static libs
)

if (APPLE)
  install(TARGETS prova-tools tash noch-lib
    LIBRARY DESTINATION prova-ui.app/Contents/Frameworks
  )

  set_target_properties(prova-tools tash noch-lib PROPERTIES
    INSTALL_NAME_DIR "@rpath"
    INSTALL_RPATH    "@loader_path"
  )
endif()


add_subdirectory(ui)
