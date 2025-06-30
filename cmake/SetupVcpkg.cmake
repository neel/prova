# cmake/SetupVcpkg.cmake
cmake_minimum_required(VERSION 3.21)

# ------------------------------------------------------------
# 1. Figure out where vcpkg should live
# ------------------------------------------------------------
get_filename_component(_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(_CANDIDATES "")

# 1a. If the user defined a VCPKG_ROOT, try that first
if(DEFINED ENV{VCPKG_ROOT})
    list(APPEND _CANDIDATES "$ENV{VCPKG_ROOT}")
endif()

# 1b. Fallback to in-repo clone
list(APPEND _CANDIDATES "${_REPO_ROOT}/vcpkg")

# Choose the first candidate that really contains the toolchain file
set(_VCPKG_DIR "")
foreach(_cand IN LISTS _CANDIDATES)
    if(EXISTS "${_cand}/scripts/buildsystems/vcpkg.cmake")
        set(_VCPKG_DIR "${_cand}")
        break()
    endif()
endforeach()

# If none of them worked, pick the in-repo path and clone into it
if(_VCPKG_DIR STREQUAL "")
    set(_VCPKG_DIR "${_REPO_ROOT}/vcpkg")
endif()

set(_VCPKG_TOOLCHAIN "${_VCPKG_DIR}/scripts/buildsystems/vcpkg.cmake")

# ------------------------------------------------------------
# 2. Clone vcpkg if it is not present
# ------------------------------------------------------------
if(NOT EXISTS "${_VCPKG_TOOLCHAIN}")
    message(STATUS "vcpkg not found – cloning into '${_VCPKG_DIR}'")

    # Find git
    find_program(GIT_EXECUTABLE git REQUIRED)

    execute_process(
        COMMAND "${GIT_EXECUTABLE}" clone --filter=blob:none --depth 1 https://github.com/microsoft/vcpkg "${_VCPKG_DIR}"
        RESULT_VARIABLE _git_status
    )
    if(_git_status)
        message(FATAL_ERROR "git clone of vcpkg failed (error code ${_git_status})")
    endif()
endif()

# ------------------------------------------------------------
# 3. Bootstrap vcpkg (runs only if 'vcpkg' executable is missing)
# ------------------------------------------------------------
set(_VCPKG_BAT "${_VCPKG_DIR}/bootstrap-vcpkg.bat")
set(_VCPKG_SH  "${_VCPKG_DIR}/bootstrap-vcpkg.sh")
if(WIN32)
    set(_VCPKG_EXE "${_VCPKG_DIR}/vcpkg.exe")
else()
    set(_VCPKG_EXE "${_VCPKG_DIR}/vcpkg")
endif()
if(NOT EXISTS "${_VCPKG_EXE}")
    if(WIN32)
        if(NOT EXISTS "${_VCPKG_BAT}")
            message(FATAL_ERROR
                "Expected bootstrap script not found at:\n  ${_VCPKG_BAT}\n"
                "Did the clone fail? Check your network or Git version.")
        endif()
        execute_process(
            COMMAND "${_VCPKG_BAT}" -disableMetrics
            WORKING_DIRECTORY "${_VCPKG_DIR}"
        )
    else()
        if(NOT EXISTS "${_VCPKG_SH}")
            message(FATAL_ERROR
                "Expected bootstrap script not found at:\n  ${_VCPKG_SH}")
        endif()
        execute_process(
            COMMAND bash "${_VCPKG_SH}" -disableMetrics
            WORKING_DIRECTORY "${_VCPKG_DIR}"
        )
    endif()

    # verify bootstrap produced the executable
    if(NOT EXISTS "${_VCPKG_EXE}")
        message(FATAL_ERROR "vcpkg bootstrap failed; see messages above.")
    endif()
endif()

# ------------------------------------------------------------
# 4. Export toolchain path for this configure *and* the cache
# ------------------------------------------------------------
set(CMAKE_TOOLCHAIN_FILE "${_VCPKG_TOOLCHAIN}" CACHE FILEPATH "vcpkg toolchain file" FORCE)
message(STATUS "Using vcpkg toolchain: ${CMAKE_TOOLCHAIN_FILE}")
