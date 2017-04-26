set (RPARSER_SOURCES
    "Chart.cpp"
    "MetaData.cpp"
    "NoteData.cpp"
    "NoteLoader.cpp"
    "NoteLoaderBMS.cpp"
    "NoteLoaderVOS.cpp"
    "Song.cpp"
    "TimingData.cpp"
    "util.cpp")

set (RPARSER_HEADERS
    "Chart.h"
    "MetaData.h"
    "NoteData.h"
    "NoteLoader.h"
    "NoteLoaderBMS.h"
    "NoteLoaderVOS.h"
    "Song.h"
    "TimingData.h"
    "util.h")

# desired output is library
add_library(rparser ${RPARSER_SOURCES} ${RPARSER_HEADERS})