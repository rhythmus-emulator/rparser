set (RPARSER_TEST_SOURCES
    "main.cpp")

set (RPARSER_TEST_HEADERS
    )


if (NOT MSVC)
find_package(iconv)
else ()
set(ICONV_LIB '')
set(ICONV_INCLUDE_DIR '')
endif ()
find_package(zip)
find_package(zlib)
find_package(openssl)



include_directories(${RPARSER_INCLUDE_DIR} ${RUTIL_INCLUDE_DIR} ${ZIP_INCLUDE_DIR})

# executable file
add_executable(rparser_test ${RPARSER_TEST_SOURCES} ${RPARSER_TEST_HEADERS})


# refer 
target_link_libraries(rparser_test rparser)
target_link_libraries(rparser_test rutil)
if (zlib_FOUND AND zip_FOUND)
target_link_libraries(rparser_test ${ZIP_LIBRARY})
target_link_libraries(rparser_test ${ZLIB_LIBRARY})
endif ()
if (openssl_FOUND)
target_link_libraries(rparser_test ${OPENSSL_LIBRARY})
endif()