project(rparser_python)

find_package(PythonLibs)
if (NOT PYTHONLIBS_FOUND)
    message(FATAL_ERROR "No python lib found")
endif ()

set (RPARSER_PYTHONLIB_SOURCES
    "main.cpp")

set (RPARSER_PYTHONLIB_HEADERS
    )

include_directories(
    ${LIB_INCLUDE_DIR}
    ${RPARSER_INCLUDE_DIR}
    ${RUTIL_INCLUDE_DIR}
    ${ZIP_INCLUDE_DIR}
    ${OpenSSL_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/pybind11/include
	)

add_subdirectory(pybind11)

# pybind
#pybind11_add_module(rparser_py ${RPARSER_PYTHONLIB_SOURCES})

# pybind with target_link_libraries
add_library(rparser_py MODULE main.cpp)
target_link_libraries(rparser_py rparser ${PYTHON_LIBRARIES})
if (ZLIB_FOUND AND ZIP_FOUND)
  target_link_libraries(rparser_py ${ZIP_LIBRARY})
  target_link_libraries(rparser_py ${ZLIB_LIBRARY})
endif ()
if (OPENSSL_FOUND)
  target_link_libraries(rparser_py ${OPENSSL_LIBRARY})
endif()
set_target_properties(rparser_py PROPERTIES PREFIX "${PYTHON_MODULE_PREFIX}"
                                         SUFFIX "${PYTHON_MODULE_EXTENSION}")