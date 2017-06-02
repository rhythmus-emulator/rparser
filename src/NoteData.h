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
    // @description undefined, usually for UnTappable object.
    NOTETYPE_EMPTY,
    // @description general tappable note
    NOTETYPE_TAP,
    // @description head of longnote
    NOTETYPE_CHARGE,
    // @description head of hell-charge note
    NOTETYPE_HCHARGE,
    // @description head of tick-charge note, like pump
    NOTETYPE_TCHARGE,
    // @description not sounds, only changes keysound
    NOTETYPE_INVISIBLE,
    NOTETYPE_MINE,
    NOTETYPE_SHOCK,
    NOTETYPE_AUTOPLAY,
    // @description drawn but not judged
    NOTETYPE_FAKE,
    // @description not drawn no miss, but judged
    NOTETYPE_EXTRA,
    // @description free score area, like osu / taigo
    NOTETYPE_FREE
}

/*
 * @description
 * An object in BGM/BGA/Tappable channel
 * Mostly these object has type, position, value.
 * You may need to process these objects properly to make playable objects.
 */
struct Note {
    NoteType type;

    // @description
    // does nothing in BMS game.
    // x / y means position in touch based game
    // x means col number in BMS BGM channel.
    int x,y;

    int iValue;         // mostly channel index
    int iDuration;      // row duration (for LN)

    float fVolume;
    int iPitch;

    // @description time information (won't be saved)
    // won't be filled until you call FillTimingData()
    float fTime;
    float fDuration;    // duration of msec

    // @description combo per note (generally 1)
    // it may be over 1 in case of tick-longnote.
    int iCombo;

    Note() : x(0), y(0), iValue(0), iDuration(0),
        fVolume(0), iPitch(0),
        fTime(0), fDuration(0), iCombo(1) {}
};

// @description automatic notes only used for BGM
struct AutoNote {
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
     * In case of osu(or touch based)/taigo, all note in First track; 
     * - so no meaning in track - but should use multiple track when multitouch. (up to 10)
     */
    typedef std::map<int, TapNote> Track;
    typedef std::map<int, TapNote>::iterator trackiter;
    typedef std::map<int, TapNote>::const_iterator const_trackiter;

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
    void AddNote(const Note& n);
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
    void ApplyResolutionRatio(float fRatio);
private:
    std::vector<Track> m_Tracks;
    std::vector<std::pair<int, Note>> m_BGMTrack;
};

}

#endif

/*
 * Tip for compiling notedata into playable objects:
 * - Separate Autoplay objects from playable objects
 *   (ex: BGM, BGA, INVISIBLE, FAKE, etc...)
 *   Consider these autoplay objects as objects which 
 *   do not affected when calculating Judgements - 
 *   just process them when timing reached.
 */