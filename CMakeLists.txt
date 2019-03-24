cmake_minimum_required(VERSION 3.0)
project(rparser_solution)

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
set(LIB_DIR "${CMAKE_SOURCE_DIR}/../rparser-lib/prebuilt/x86-MT")
set(LIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../rparser-lib/include")
if (NOT MSVC)
	find_package(iconv)
	find_package(zip PATHS ${LIB_DIR})
	find_package(zlib PATHS ${LIB_DIR})
	find_package(openssl PATHS ${LIB_DIR})
else()
	set(iconv_LIB "")
	set(iconv_INCLUDE_DIR "")
	set(zlib_FOUND 1)
	set(zlib_LIBRARY "${LIB_DIR}/zlib.lib")
	set(zlib_INCLUDE_DIR "")
	set(zip_FOUND 1)
	set(zip_LIBRARY "${LIB_DIR}/zip.lib")
	set(zip_INCLUDE_DIR "")
	set(openssl_FOUND 1)
	set(openssl_LIBRARY "${LIB_DIR}/libcryptoMD.lib" "${LIB_DIR}/libsslMD.lib")
	set(openssl_INCLUDE_DIR "")
endif ()

if (zlib_FOUND AND zip_FOUND)
	add_definitions(-DUSE_ZLIB)
endif ()

if (openssl_FOUND)
	add_definitions(-DUSE_OPENSSL)
endif ()

# project: library
add_subdirectory("${PROJECT_SOURCE_DIR}/src")

# project: test
# (if not, just add src)
add_subdirectory("${PROJECT_SOURCE_DIR}/test")