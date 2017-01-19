/*
 * by @lazykuna, MIT License.
 */

#include <string>

namespace rparser {

namespace NoteLoaderBMS {
    public:

    bool LoadNoteData( const std::string& fpath, NoteData& nd, bool processCmd=true );
    bool ParseNoteData( const char* p, int iLen, Notedata& nd, bool processCmd=true );

    bool LoadMetaData( const std::string& fpath, MetaData& md, bool processCmd=true );
    bool ParseMetaData( const char* p, int iLen, MetaData& md, bool processCmd=true );
}

}