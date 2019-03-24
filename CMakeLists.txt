cmake_minimum_required(VERSION 3.0)
project(rparser)

# project common settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/lib")
if (MSVC)
	#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	set(CMAKE_CXX_FLAGS_RELEASE "/MT")
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# COMMENT:
# To make gtest build in MDd,
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# enable this line.

# setting for common lib / include
set(LIB_DIR "${CMAKE_SOURCE_DIR}/../prebuilt")
set(LIB_INCLUDE "${CMAKE_SOURCE_DIR}/../include")
if (NOT MSVC)
	find_package(iconv)
else()
	set(ICONV_LIB "")
	set(ICONV_INCLUDE_DIR "")
endif ()
find_package(zip PATHS ${LIB_DIR})
find_package(zlib PATHS ${LIB_DIR})
find_package(openssl PATHS ${LIB_DIR})

# project: library
add_subdirectory("${PROJECT_SOURCE_DIR}/src")

# project: test
# (if not, just add src)
add_subdirectory("${PROJECT_SOURCE_DIR}/test")