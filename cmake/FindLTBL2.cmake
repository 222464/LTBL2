# - Find LTBL2
# Find the LTBLs includes and library
#
#   LTBL2_INCLUDE_DIR - LBTL2 include directory.
#   LTBL2_LIBRARY     - LBTL2 library.
#
find_path(LTBL_LIBRARYINCLUDE_DIR
    LTBL/LightSystem.h
    PATH_SUFFIXES include
    PATHS /usr /usr/local /opt/local)

find_library(LTBL_LIBRARY
    ltbl
    PATH_SUFFIXES lib lib64
    PATHS /usr /usr/local /opt/local)

if(NOT LTBL_INCLUDE_DIR OR NOT LTBL_LIBRARY)
    message(FATAL_ERROR "LTBL not found.")
else()
    message("LTBL found: ${LTBL_INCLUDE_DIR}")
endif()
