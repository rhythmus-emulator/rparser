# Locate libzip
# This module defines
# LIBZIP_LIBRARY
# LIBZIP_FOUND, if false, do not try to link to libzip
# LIBZIP_INCLUDE_DIR, where to find the headers
#

FIND_PATH(ZIP_INCLUDE_DIR zip.h
    HINTS
    ${CMAKE_SOURCE_DIR}/../rparser-lib/include
    PATHS
    $ENV{LIBZIP_DIR}/include
    $ENV{LIBZIP_DIR}
    /usr/local/include
    /usr/include
)

FIND_LIBRARY(ZIP_LIBRARY
    NAMES libzip zip
    HINTS
    ${CMAKE_SOURCE_DIR}/../rparser-lib/prebuilt/linux-x86
    PATHS
    $ENV{LIBZIP_DIR}/lib
    $ENV{LIBZIP_DIR}
    /usr/local/lib
    /usr/lib
)

SET(ZIP_FOUND "NO")
IF(ZIP_LIBRARY AND ZIP_INCLUDE_DIR)
    SET(ZIP_FOUND "YES")
ENDIF(ZIP_LIBRARY AND ZIP_INCLUDE_DIR)
