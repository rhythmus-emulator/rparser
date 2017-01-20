/*
 * by @lazykuna, MIT License.
 */

#include <string>

namespace rparser {

// @description global error reason, defined in NoteLoader.cpp
extern std::string ErrorString;

namespace NoteLoaderBMS {
    bool LoadChart( const std::string& fpath, Chart& chart, bool processCmd=true );
    bool ParseChart( const std::string& fpath, Chart& chart, bool processCmd=true );

    bool ParseNoteData( const char* p, int iLen, NoteData& nd, bool processCmd=true );
    bool LoadMetaData( const std::string& fpath, MetaData& md, bool processCmd=true );
    bool ParseMetaData( const char* p, int iLen, MetaData& md, bool processCmd=true );
    bool ParseTimingData( const char* p, int iLen, TimingData& td, bool processCmd=true );
}

}

//
// expected to be used by NoteLoader module!
//