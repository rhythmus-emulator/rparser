cmake_minimum_required(VERSION 3.0)
project(rparser)

# project common settings
set(RPARSER_INCLUDE_DIRS
    "include")
include_directories(${RPARSER_INCLUDE_DIRS})
link_directories("lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/lib")

# add subdirectories
add_subdirectory("${PROJECT_SOURCE_DIR}/src")
#add_subdirectory("${PROJECT_SOURCE_DIR}/test")