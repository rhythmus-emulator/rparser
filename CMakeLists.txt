cmake_minimum_required(VERSION 3.0)
project(rparser)

# project common settings
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/lib")

# rutil & common libraries
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/rhythmus-lib")
add_subdirectory("${PROJECT_SOURCE_DIR}/rhythmus-lib")

# add subdirectories
add_subdirectory("${PROJECT_SOURCE_DIR}/src")
add_subdirectory("${PROJECT_SOURCE_DIR}/test")