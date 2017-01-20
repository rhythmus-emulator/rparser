/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_NOTELOADER_H
#define RPARSER_NOTELOADER_H

class Chart;
class NoteData;
class MetaData;
class TimingData;

namespace rparser {

// @description global error reason, defined in NoteLoader.cpp
extern std::string ErrorString;

// general note reader.
bool LoadChart( const std::string& fpath, Chart& chart );
bool ParseChart( const std::string& fpath, Chart& chart );

bool ParseNoteData( const char* p, int iLen, NoteData& nd );
bool LoadMetaData( const std::string& fpath, MetaData& md );
bool ParseMetaData( const char* p, int iLen, MetaData& md );
bool ParseTimingData( const char* p, int iLen, TimingData& td );

}

#endif