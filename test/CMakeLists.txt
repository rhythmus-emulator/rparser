set (RPARSER_TEST_SOURCES
    "main.cpp")

set (RPARSER_TEST_HEADERS
    )

include_directories("../src")

# executable file
add_executable(rparser_test ${RPARSER_TEST_SOURCES} ${RPARSER_TEST_HEADERS})


# refer 
target_link_libraries(rparser_test rparser)
target_link_libraries(rparser_test "zip")
target_link_libraries(rparser_test "zlib")
target_link_libraries(rparser_test "libsslMD")
target_link_libraries(rparser_test "libcryptoMD")