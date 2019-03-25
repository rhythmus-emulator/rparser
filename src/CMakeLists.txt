project(rparser_lib)

# prepare necessary source / header
set (RPARSER_SOURCES
    "Chart.cpp"
    "ChartLoader.cpp"
    "ChartLoaderBMS.cpp"
    "ChartLoaderVOS.cpp"
    "ChartWriter.cpp"
    "ChartUtil.cpp"
    "MetaData.cpp"
    "Resource.cpp"
    "Song.cpp"
    "TempoData.cpp"
    "rutil.cpp")

set (RPARSER_HEADERS
    "Chart.h"
    "ChartLoader.h"
    "ChartWriter.h"
    "ChartUtil.h"
    "MetaData.h"
    "Resource.h"
    "Song.h"
    "TempoData.h"
    "Error.h"
    "rutil.h"
    "common.h")

# declare RPARSER_INCLUDE_DIR variable to parent scope
# XXX: is it necessary? should remove?
set (RPARSER_INCLUDE_DIR "${CMAKE_CURRENT_LIST_DIR}" PARENT_SCOPE)


include_directories(
	${LIB_INCLUDE_DIR}
	${RPARSER_INCLUDE_DIR}
	${GTEST_HEADERS}
	${RUTIL_INCLUDE_DIR}
  ${ZIP_INCLUDE_DIR}
  ${OpenSSL_INCLUDE_DIR}
	)
#link_directories(${ZIP_LIB} ${ZLIB_LIB} ${OPENSSL_LIB} ${ICONV_LIB})


add_library(rparser STATIC ${RPARSER_SOURCES} ${RPARSER_HEADERS})
