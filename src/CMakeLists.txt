# prepare necessary source / header
set (RPARSER_SOURCES
    "Chart.cpp"
	"ChartData.cpp"
    "ChartLoader.cpp"
    "ChartLoaderBMS.cpp"
    "ChartLoaderVOS.cpp"
    "ChartWriter.cpp"
    "MetaData.cpp"
    "Resource.cpp"
    "Song.cpp"
    "TimingData.cpp"
	"rutil.cpp")

set (RPARSER_HEADERS
    "Chart.h"
	"ChartData.h"
    "ChartLoader.h"
    "ChartWriter.h"
    "MetaData.h"
	"Resource.h"
    "Song.h"
    "TimingData.h"
	"rutil.h")

# declare RPARSER_INCLUDE_DIR variable to parent scope
# XXX: is it necessary? should remove?
set (RPARSER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}" PARENT_SCOPE)


if (zip_FOUND AND zlib_FOUND)
	add_definitions(-DUSE_ZLIB)
else()
	set(ZIP_INCLUDE_DIR "")
	set(ZLIB_INCLUDE_DIR "")
endif()

if (openssl_FOUND)
	add_definitions(-DUSE_OPENSSL)
else()
	set(OPENSSL_INCLUDE_DIR "")
endif()


include_directories(${ZIP_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${ICONV_INCLUDE_DIR} ${RUTIL_INCLUDE_DIR})
#link_directories(${ZIP_LIB} ${ZLIB_LIB} ${OPENSSL_LIB} ${ICONV_LIB})


add_library(rparser ${RPARSER_SOURCES} ${RPARSER_HEADERS})


# if proceed with test
if (rparser_test)
	add_subdirectory("test")
endif()