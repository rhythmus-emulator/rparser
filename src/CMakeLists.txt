set (RPARSER_SOURCES
    "Chart.cpp"
    "MetaData.cpp"
    "NoteData.cpp"
    "ChartLoader.cpp"
    "ChartLoaderBMS.cpp"
    "ChartLoaderVOS.cpp"
    "ChartWriter.cpp"
    "Song.cpp"
    "TimingData.cpp"
    "util.cpp")

set (RPARSER_HEADERS
    "Chart.h"
    "MetaData.h"
    "NoteData.h"
    "ChartLoader.h"
    "ChartWriter.h"
    "Song.h"
    "TimingData.h"
    "util.h")

# desired output is library
add_library(rparser ${RPARSER_SOURCES} ${RPARSER_HEADERS})