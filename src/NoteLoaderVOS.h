/*
 * by @lazykuna, MIT License.
 */

#include <string>

namespace rparser {

namespace NoteLoaderVOS {
    public:

    bool LoadNoteData( const std::string& fpath, NoteData& nd );
    bool ParseNoteData( const char* p, int iLen, Notedata& nd );

    bool LoadMetaData( const std::string& fpath, MetaData& md );
    bool ParseMetaData( const char* p, int iLen, MetaData& md );
}

}