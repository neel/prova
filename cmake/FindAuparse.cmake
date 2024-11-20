# Try to find libraries for auparse

# Set the minimum required version of auparse, if any
#set(AUPARSE_MIN_VERSION "x.x.x")

# Find auparse headers
find_path(AUPARSE_INCLUDE_DIR
  NAMES auparse.h
  PATHS /usr/include /usr/local/include
  PATH_SUFFIXES auparse
)

# Find auparse library
find_library(AUPARSE_LIBRARY
  NAMES auparse
  PATHS /usr/lib /usr/local/lib
)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set AUPARSE_FOUND to TRUE if
# all listed variables are TRUE
find_package_handle_standard_args(AUParse DEFAULT_MSG
  AUPARSE_LIBRARY AUPARSE_INCLUDE_DIR)

if(AUPARSE_FOUND)
  set(AUPARSE_LIBRARIES ${AUPARSE_LIBRARY})
  set(AUPARSE_INCLUDE_DIRS ${AUPARSE_INCLUDE_DIR})
endif()

mark_as_advanced(AUPARSE_INCLUDE_DIR AUPARSE_LIBRARY)

