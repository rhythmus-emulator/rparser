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
    "util.cpp")

set (RPARSER_HEADERS
    "Chart.h"
    "MetaData.h"
    "NoteData.h"
    "ChartLoader.h"
    "ChartWriter.h"
    "Song.h"
    "TimingData.h"
    "util.h")


if (NOT MSVC)
find_package(iconv)
else()
set(ICONV_LIB '')
set(ICONV_INCLUDE_DIR '')
endif ()
find_package(zip)
find_package(zlib)
find_package(openssl)


include_directories(${ZIP_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${ICONV_DIR} ${RUTIL_DIR})
link_directories(${ZIP_LIB} ${ZLIB_LIB} ${OPENSSL_LIB} ${ICONV_LIB})


# desired output is library
add_library(rparser ${RPARSER_SOURCES} ${RPARSER_HEADERS})