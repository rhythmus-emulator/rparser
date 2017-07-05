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


include_directories(${ZIP_INCLUDE_DIR} ${ZLIB_INCLUDE_DIR} ${OPENSSL_INCLUDE_DIR} ${ICONV_DIR})
#link_directories(${ZIP_LIB} ${ZLIB_LIB} ${OPENSSL_LIB} ${ICONV_LIB})


# executable file
add_executable(rparser_test ${RPARSER_TEST_SOURCES} ${RPARSER_TEST_HEADERS})


# refer 
target_link_libraries(rparser_test rparser)
target_link_libraries(rparser_test rutil)
target_link_libraries(rparser_test "zip")
target_link_libraries(rparser_test "zlib")
target_link_libraries(rparser_test "libsslMD")
target_link_libraries(rparser_test "libcryptoMD")