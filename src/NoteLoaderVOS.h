/*
 * by @lazykuna, MIT License.
 */

#include <string>

namespace rparser {
    
// @description global error reason, defined in NoteLoader.cpp
extern std::string ErrorString;

namespace NoteLoaderVOS {
    bool LoadChart( const std::string& fpath, Chart& chart );
    bool ParseChart( const char* p, int iLen, Chart& chart );

    bool ParseNoteData( const char* p, int iLen, NoteData& nd );
    bool LoadMetaData( const std::string& fpath, MetaData& md );
    bool ParseMetaData( const char* p, int iLen, MetaData& md );
    bool ParseTimingData( const char* p, int iLen, TimingData& td );
}

}

//
// expected to be used by NoteLoader module!
//