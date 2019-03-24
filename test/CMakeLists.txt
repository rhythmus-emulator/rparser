project(rparser_test)

set (RPARSER_TEST_SOURCES
    "main.cpp")

set (RPARSER_TEST_HEADERS
    )

set (GTEST_HEADERS
	"${CMAKE_SOURCE_DIR}/../googletest/googletest/include"
    )


# lib: gtest_main
add_subdirectory("${CMAKE_SOURCE_DIR}/../googletest" "${CMAKE_CURRENT_SOURCE_DIR}/lib")

include_directories(
	${LIB_INCLUDE_DIR}
	${RPARSER_INCLUDE_DIR}
	${GTEST_HEADERS}
	${RUTIL_INCLUDE_DIR}
	${zip_INCLUDE_DIR}
	${openssl_INCLUDE_DIR}
	)

# executable file
add_executable(rparser_test ${RPARSER_TEST_SOURCES} ${RPARSER_TEST_HEADERS})

# refer
target_link_libraries(rparser_test rparser)
if (zlib_FOUND AND zip_FOUND)
target_link_libraries(rparser_test ${zip_LIBRARY})
target_link_libraries(rparser_test ${zlib_LIBRARY})
endif ()
if (openssl_FOUND)
target_link_libraries(rparser_test ${openssl_LIBRARY})
endif()
target_link_libraries(rparser_test gtest)