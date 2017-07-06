project(rutil)

if ($ENV{RUTIL_ALL})

    set (RUTIL_SOURCES
        "rutil.Image.cpp"
        "rutil.Sound.cpp"
        "rutil.cpp")

    set (RUTIL_HEADERS
        "rutil.Image.h"
        "rutil.Sound.h"
        "rutil.h")

    find_package(libpng)
    find_package(ffmpeg)
    find_package(libogg)

else()

    set (RUTIL_SOURCES
        "rutil.cpp")

    set (RUTIL_HEADERS
        "rutil.h")

endif()

set (RUTIL_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}" PARENT_SCOPE)

# MSVC won't use iconv
if (NOT MSVC)
find_package(iconv)
endif()
find_package(zip)
find_package(zlib)

include_directories(
    ${ZLIB_INCLUDE_DIR} ${ZIP_INCLUDE_DIR} ${ICONV_INCLUDE_DIR}
    )

# desired output is library
add_library(rutil STATIC ${RUTIL_SOURCES} ${RUTIL_HEADERS})