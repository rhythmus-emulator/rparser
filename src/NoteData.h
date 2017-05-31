/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_NOTEDATA_H
#define RPARSER_NOTEDATA_H

#include <map>
#include <vector>
#include "TimingData.h"

namespace rparser {


/*
 * @description
 * holds result for tapping note,
 * but it may not be necessary if you're not going to play/visualize, in most cases.
 * but in case we need ...
 * performance won't be affected much by adding this struct, I believe.
 * but maybe we need to modify this structure, in case of addition of more features.
 * That may be a little nasty thing...
 */
struct NoteResult {
    // @description is note had been properly tapped
    bool isTapped;
    // @description is note had been properly released
    bool isReleased;
    // @description is player holding note?
    bool isHeld;
    // @description should note be in hidden state?
    bool bHidden;
    int iTapOffset;
    // @description relesing time of note
    int iReleaseOffset;
    // @description holded time in case of hold note
    int iDuration;
}

enum NoteType {
    // @description 
    NOTETYPE_EMPTY,
    // @description general tappable note
    NOTETYPE_TAP,
    // @description head of longnote
    NOTETYPE_CHARGE,
    // @description head of hell-charge note
    NOTETYPE_HCHARGE,
    // @description head of tick-charge note, like pump
    NOTETYPE_TCHARGE,
    NOTETYPE_MINE,
    NOTETYPE_SHOCK,
    NOTETYPE_AUTOPLAY,
    NOTETYPE_FAKE,
    // @description free score area, like osu / taigo
    NOTETYPE_FREE
}

enum NoteChannelType {
    NOTECHANNEL_WAV,
    NOTECHANNEL_MIDI
}

/*
 * @description
 * A soundable/tappable object, mostly playable by player. 
 * (Not includes command or BGA info here!)
 */
struct Note {
    NoteResult result;
    NoteType type;

    // @description occuring damage in case of MINE object (0 ~ 1)  
    float fDamage;

    // @description designates sound playing channel
    NoteChannelType channeltype;
    int iChannel;
    float fVolume;
    int iDuration;      // row duration
    int iPitch;

    // @description x means track, y means bar - in Track based game.
    int x;
    int y;

    // @description time information won't be filled until you call FillTimingData()
    float fTime;
    float fDuration;    // duration of msec

    // @description combo per note - it may be over 1 in case of tick-longnote.
    int iCombo;

    // @description for user-customizing metadata, basically filled with 0. use at your own. 
    void *p;
};

// @description notes only used for BGM
struct NoteBGM {
    int iChannel;
    int BGAcol;     // for bmse
    int bSound;     // is it soundable? (generally yes)

    // @description 
    // this value makes keysound change in targeted channel
    // used for BMSE invisible note channel
    int iKeysoundChannel;
};

class NoteData {
public:
    /*
     * @description
     * Notedata consists with multiple track(lane), and Track contains Notes.
     * And each note is indexed with 'Row'
     * In case of osu/taigo, all note in First track; no meaning in track.
     */
    typedef std::map<int, Note> Track;
    typedef std::map<int, Note>::iterator trackiter;
    typedef std::map<int, Note>::const_iterator const_trackiter;

    trackiter begin(int tracknum) { return tracks[tracknum]->begin(); };
    trackiter end(int tracknum) { return tracks[tracknum]->end(); };
    trackiter lower_bound(int tracknum, int row) { return tracks[tracknum]->lower_bound(row); };
    trackiter upper_bound(int tracknum, int row) { return tracks[tracknum]->upper_bound(row); };
    const_trackiter begin(int tracknum) { return tracks[tracknum]->begin(); };
    const_trackiter end(int tracknum) { return tracks[tracknum]->end(); };
    const_trackiter lower_bound(int tracknum, int row) { return tracks[tracknum]->lower_bound(row); };
    const_trackiter upper_bound(int tracknum, int row) { return tracks[tracknum]->upper_bound(row); };

    /*
     * @description
     * for who wants to iterate Note objects by beats, not considering Track.
     */
    void SearchAllNotes(std::vector<trackiter>& notelist);
    void SearchAllNotes(std::vector<trackiter>& notelist, int iStartRow, int iEndRow, bool bInclusive);

    // @description fill all note's timing data from beat data.
    void FillTimingData(const TimingData& td);


    /*
     * metadata utilities
     */
    int GetTrackCount();
    void SetTrackCount(int iTrackCnt);
    int GetNoteCount();
    int GetNoteCount(int iTrackNum);
    // @description 
    int GetLastNoteRow();
    int GetLastNoteTime();
    bool IsHoldNoteAtRow(int track, int row);
    NoteType GetNoteTypeAtRow(int track, int row);
    Note* GetNoteAtRow(int track, int row);
    bool IsRangeEmpty(int track, int iStartRow, int iEndRow);
    bool IsRowEmpty(int row);
    bool IsTrackEmpty(int track);
    bool IsEmpty();


    /*
     * modification utilities
     */
    void RemoveNotes();
    void RemoveNotes(int iStartRow, int iEndRow, bool bInclusive);
    void Clear();
    void ClearRange(int iStartRow, int iEndRow);
    void CopyRange(int rowFromBegin, int rowFromLength, int rowToBegin);
    void CopyRange(const NoteData& nd);
    void CopyRange(const NoteData& nd, int iStartRow, int iEndRow);
    void TrackMapping(int tracknum, int *trackmap);
    void TrackRandom();
    void TrackSRandom();
    void TrackHRandom();
    void TrackRRandom();
    void TrackMirror();
    // @description useful for iidx(DP) style
    void TrackRandom(int side);
    void TrackSRandom(int side);
    void TrackHRandom(int side);
    void TrackRRandom(int side);
    void TrackMirror(int side);
    void TrackFlip();
    

    NoteData();
    ~NoteData();
    std::string const toString();
private:
    std::vector<Track> m_Tracks;
    std::vector<NoteBGM> m_BGMTrack;
};

}

#endif