cmake_minimum_required(VERSION 3.0)
project(rhythmus_bin)

set (RPARSER_SOURCES
    "src/Chart.cpp"
    "src/MetaData.cpp"
    "src/NoteData.cpp"
    "src/NoteLoader.cpp"
    "src/NoteLoaderBMS.cpp"
    "src/NoteLoaderVOS.cpp"
    "src/Song.cpp"
    "src/TimingData.cpp"
    "src/util.cpp")
add_executable(rhythmus_bin ${RPARSER_SOURCES})

set(RPARSER_INCLUDE_DIRS
    "src")
include_directories(${RPARSER_INCLUDE_DIRS})




