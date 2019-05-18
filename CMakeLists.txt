cmake_minimum_required(VERSION 3.0)
project(rparser_solution)

# project common settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/lib")
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
set(CMAKE_CXX_STANDARD 14)
if (MSVC)
	#set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	#set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
	#set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	set(CMAKE_CXX_FLAGS_RELEASE "/MT /Ot")
	set(CMAKE_CXX_FLAGS_DEBUG "/MTd /Zi /Od")
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Build path configuration (generic no-config case e.g. mingw)
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin" )
set( CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib" )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib" )
# Build path configuration (multi-config builds e.g. msvc)
foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
    string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
    set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_SOURCE_DIR}/bin_${OUTPUTCONFIG}" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_SOURCE_DIR}/lib_${OUTPUTCONFIG}" )
    set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} "${CMAKE_SOURCE_DIR}/lib_${OUTPUTCONFIG}" )
endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )

# COMMENT:
# To make gtest build in MDd,
# set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
# enable this line.

# setting for common lib / include
set(LIB_DIR "${CMAKE_SOURCE_DIR}/../rparser-lib/prebuilt/x86-MT")
set(LIB_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/../rparser-lib/include")
if (NOT MSVC)
  find_package(Iconv)
  find_package(ZIP)
  find_package(ZLIB)
  find_package(OpenSSL)
  set(OPENSSL_LIBRARY ${OPENSSL_CRYPTO_LIBRARY})
else()
  set(Iconv_LIB "")
  set(Iconv_INCLUDE_DIR "")
  set(ZLIB_FOUND "YES")
  set(ZLIB_LIBRARY "${LIB_DIR}/zlib.lib")
  set(ZLIB_INCLUDE_DIR "")
  set(ZIP_FOUND "YES")
  set(ZIP_LIBRARY "${LIB_DIR}/zip.lib")
  set(ZIP_INCLUDE_DIR "")
  set(OPENSSL_FOUND "YES")
  set(OPENSSL_LIBRARY "${LIB_DIR}/libcryptoMD.lib" "${LIB_DIR}/libsslMD.lib")
  set(OPENSSL_INCLUDE_DIR "")
endif ()

if (ZLIB_FOUND AND ZIP_FOUND)
  message("Archive file feature available.")
	add_definitions(-DUSE_ZLIB)
endif ()

if (OPENSSL_FOUND)
  message("OpenSSL Found.")
	add_definitions(-DUSE_OPENSSL)
endif ()

# project: library
add_subdirectory("${PROJECT_SOURCE_DIR}/src")

# project: test
# (if not, just add src)
add_subdirectory("${PROJECT_SOURCE_DIR}/test")
