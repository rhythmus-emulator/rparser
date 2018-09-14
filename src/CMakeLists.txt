set (RPARSER_SOURCES
    "Chart.cpp"
    "MetaData.cpp"
    "NoteData.cpp"
    "ChartLoader.cpp"
    "ChartLoaderBMS.cpp"
    "ChartLoaderVOS.cpp"
    "ChartWriter.cpp"
    "Song.cpp"
    "TimingData.cpp"
	"rutil.cpp")

set (RPARSER_HEADERS
    "Chart.h"
    "MetaData.h"
    "NoteData.h"
    "ChartLoader.h"
    "ChartWriter.h"
    "Song.h"
    "TimingData.h"
	"rutil.h")

set (RPARSER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}" PARENT_SCOPE)

if (NOT MSVC)
find_package(iconv)
else()
set(ICONV_LIB '')
set(ICONV_INCLUDE_DIR '')
endif ()
find_package(zip)
find_package(zlib)
find_package(openssl)

if (zip_FOUND AND zlib_FOUND)
add_definitions(-DUSE_ZLIB)
else()
set(ZIP_INCLUDE_DIR '')
set(ZLIB_INCLUDE_DIR '')
endif()
if (openssl_FOUND)
add_definitions(-DUSE_OPENSSL)
else()
set(OPENSSL_INCLUDE_DIR '')
endif()


include_directories(${ZIP_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${ICONV_INCLUDE_DIR} ${RUTIL_INCLUDE_DIR})
#link_directories(${ZIP_LIB} ${ZLIB_LIB} ${OPENSSL_LIB} ${ICONV_LIB})


# desired output is library
add_library(rparser ${RPARSER_SOURCES} ${RPARSER_HEADERS})