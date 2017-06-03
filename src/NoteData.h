/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_NOTEDATA_H
#define RPARSER_NOTEDATA_H

#include <map>
#include <vector>
#include "TimingData.h"

/*
 * I only concertrates in parsing note objects.
 * Don't care about judging and playing in this code.
 */

namespace rparser {

enum NoteType {
    // @description does nothing, maybe going to be removed.
    NOTE_EMPTY,
    // @description tappable note (user-interact)
    NOTE_TAP,
    // @description touch note, which has its position
    NOTE_TOUCH,
    // @description INVISIBLE note (which isn't judged)
    NOTE_INVISIBLE,
    // @description WAV/BGA/MIDI notes,
    // which are automatically processed in time
    NOTE_BGM,
    NOTE_BGA,
    NOTE_MIDI,
    // @description for Bms compatibility
    NOTE_BPM,
    // @description for Bms compatibility
    NOTE_STOP,
};

enum TapNoteType {
    // @description undefined, usually for UnTappable object.
    TAPNOTE_EMPTY,
    // @description general tappable note
    TAPNOTE_TAP,
    // @description head of longnote
    TAPNOTE_CHARGE,
    // @description head of hell-charge note
    TAPNOTE_HCHARGE,
    // @description head of tick-charge note, like pump
    TAPNOTE_TCHARGE,
    // @description not sounds, only changes keysound
    TAPNOTE_MINE,
    TAPNOTE_SHOCK,
    TAPNOTE_AUTOPLAY,
    // @description drawn but not judged
    TAPNOTE_FAKE,
    // @description not drawn no miss, but judged
    TAPNOTE_EXTRA,
    // @description free score area, like osu / taigo
    TAPNOTE_FREE,
};

/*
 * @description
 * An object in BGM/BGA/Tappable channel
 * Mostly these object has type, position, value.
 * You may need to process these objects properly to make playable objects.
 */
struct Note {
    NoteType nType;
    TapNoteType tType;

    int iRow;           // @description time position in row, only for editing.
    int iValue;         // mostly channel index
    int iDuration;      // row duration (for LN)

    float fVolume;
    int iPitch;

    // @description
    // This information is only for playing
    // (to reduce loss of incompatible resolution)
    float fBeat;
    // @description time information (won't be saved)
    // won't be filled until you call FillTimingData()
    float fTime;        // msec
    float fDuration;    // duration of msec

    // @description
    // does nothing in BMS gamemode, but means col number in BGM channel.
    // x / y means position in touch based gamemode.
    // x / y means command/value MIDI channel.
    int x,y;

    // @description combo per note (generally 1)
    // it may be over 1 in case of tick-longnote.
    int iCombo;

    Note() : x(0), y(0), iValue(0), iDuration(0),
        fVolume(0), iPitch(0),
        fTime(0), fDuration(0), iCombo(1) {}
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
    typedef std::vector<Note> Track;
    typedef std::vector<Note>::iterator trackiter;
    typedef std::vector<Note>::const_iterator const_trackiter;

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

    // @description calculate fBeat from iRow(after editing)
    void CalculateNoteBeat();
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
    // @description All Objects are placed in here.
    // sorted in Row->x->y
    Track m_Track;
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