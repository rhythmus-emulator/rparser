project(rparser_util)

set (UTIL_SOURCES
    "main.cpp")

include_directories(
    ${LIB_INCLUDE_DIR}
    ${RPARSER_INCLUDE_DIR}
    ${RUTIL_INCLUDE_DIR}
)

add_executable(rparser_util ${UTIL_SOURCES})

target_link_libraries(rparser_util rparser)
if (ZLIB_FOUND AND ZIP_FOUND)
  target_link_libraries(rparser_util ${ZIP_LIBRARY})
  target_link_libraries(rparser_util ${ZLIB_LIBRARY})
endif ()
if (OPENSSL_FOUND)
  target_link_libraries(rparser_util ${OPENSSL_LIBRARY})
endif()
if (Iconv_LIBRARY)
  target_link_libraries(rparser_util ${Iconv_LIBRARY})
endif()