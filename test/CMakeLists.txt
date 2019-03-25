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
  ${ZIP_INCLUDE_DIR}
  ${OpenSSL_INCLUDE_DIR}
	)

# executable file
add_executable(rparser_test ${RPARSER_TEST_SOURCES} ${RPARSER_TEST_HEADERS})

# refer
target_link_libraries(rparser_test rparser)
if (ZLIB_FOUND AND ZIP_FOUND)
  target_link_libraries(rparser_test ${ZIP_LIBRARY})
  target_link_libraries(rparser_test ${ZLIB_LIBRARY})
endif ()
if (OpenSSL_FOUND)
  target_link_libraries(rparser_test ${OpenSSL_LIBRARY})
endif()
target_link_libraries(rparser_test gtest)
