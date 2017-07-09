project(rutil)

if (DEFINED ENV{RUTIL_ALL} OR DEFINED ENV{RUTIL_SOUND})
    set (RUTIL_SOUND_SOURCE "rutil.Sound.cpp")
    set (RUTIL_SOUND_HEADER "rutil.Sound.h")
    find_package(ogg)
    find_package(flac)
    find_package(lame)
endif ()

if (DEFINED ENV{RUTIL_ALL} OR DEFINED ENV{RUTIL_IMAGE})
    set (RUTIL_IMAGE_SOURCE "rutil.Image.cpp")
    set (RUTIL_IMAGE_HEADER "rutil.Image.h")
    find_package(ffmpeg)
    find_package(png)
endif ()

set (RUTIL_SOURCES
    ${RUTIL_SOUND_SOURCE}
    ${RUTIL_IMAGE_SOURCE}
    "rutil.cpp")

set (RUTIL_HEADERS
    ${RUTIL_SOUND_HEADER}
    ${RUTIL_IMAGE_HEADER}
    "rutil.h")

set (RUTIL_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}" PARENT_SCOPE)

# MSVC won't use iconv
if (NOT MSVC)
find_package(iconv)
endif()
find_package(zip)
find_package(zlib)

include_directories(
    ${ZLIB_INCLUDE_DIR} ${ZIP_INCLUDE_DIR} ${ICONV_INCLUDE_DIR}
	${VORBIS_INCLUDE_DIR}
	${FLAC_INCLUDE_DIR}
	${LAME_INCLUDE_DIR}
    )

# desired output is library
add_library(rutil STATIC ${RUTIL_SOURCES} ${RUTIL_HEADERS})