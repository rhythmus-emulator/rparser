/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_NOTEDATA_H
#define RPARSER_NOTEDATA_H

#include <map>
#include <vector>
#include <algorithm>
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
    // @description WAV/BGA/MIDI notes,
    // which are automatically processed in time
    NOTE_BGM,
    NOTE_BGA,
    NOTE_MIDI,
    // @description for Bms compatibility
    NOTE_BMS,
    // @description resting area (for osu)
    NOTE_REST,
    NOTE_SHOCK,
};

enum NoteTapType {
    // @description undefined, usually for UnTappable object.
    TAPNOTE_EMPTY,
    // @description general tappable note
    TAPNOTE_TAP,
    // @description invisible, no damage, but scorable
    TAPNOTE_INVISIBLE,
    // @description head of longnote (various types)
    TAPNOTE_CHARGE,
    TAPNOTE_MINE,
    TAPNOTE_AUTOPLAY,
    // @description drawn but not judged
    TAPNOTE_FAKE,
    // @description not drawn no miss, but judged
    TAPNOTE_EXTRA,
    // @description free score area, like osu / taigo
    TAPNOTE_FREE,
};

enum NoteBgaType {
    NOTEBGA_EMPTY,
    NOTEBGA_BGA,
    NOTEBGA_MISS,
    NOTEBGA_LAYER1,
    NOTEBGA_LAYER2,
};

enum NoteBmsType {
    NOTEBMS_EMPTY,
    NOTEBMS_BPM,
    NOTEBMS_STOP,
    NOTEBMS_BGAOPAC,
    NOTEBMS_INVISIBLE,  // for only-keysound changing
};

/*
 * @description
 * An object in BGM/BGA/Tappable channel
 * Mostly these object has type, position, value.
 * You may need to process these objects properly to make playable objects.
 */
struct Note {
    NoteType nType;     // note main type
    int subType;        // note sub type

    int iValue;         // command value
    int iEndValue;      // value of the end (for LN)

    float fVolume;
    int iPitch;

    int iRow;           // @description time position in row, only for editing.
    int iDuration;      // row duration (for LN)
    // @description
    // This information is only for playing
    // (to reduce loss of incompatible resolution)
    float fBeat;
    float fBeatLength;  // beat duration of msec
    // @description time information (won't be saved)
    // won't be filled until you call FillTimingData()
    float fTime;        // msec
    float fTimeLength;  // msec

    // @description
    // BMS: x means track number in TapNote, col number in BGM.
    // x / y means position in touch based gamemode.
    // x track / y means command in MIDI file format.
    int x,y;

    // @description bmson attribute.
    bool restart;

    Note() : x(0), y(0), iValue(0), iDuration(0),
        fVolume(0), iPitch(0),
        fTime(0), fTimeLength(0), fBeat(0), fBeatLength(0) {}

    std::string toString();
	bool operator<(const Note &other) const;
	bool operator==(const Note &other) const;
    bool IsTappableNote();
    int GetPlayerSide();
    int GetTrack();
};


struct ChartSummaryData;
class NoteSelection;
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

	trackiter begin() { return m_Track.begin(); };
	trackiter end() { return m_Track.end(); };
	const_trackiter begin() const { return m_Track.begin(); };
	const_trackiter end() const { return m_Track.end(); };
    trackiter lower_bound(int row);
    trackiter upper_bound(int row);
    trackiter lower_bound(Note &n) { return std::lower_bound(m_Track.begin(), m_Track.end(), n); };
    trackiter upper_bound(Note &n) { return std::upper_bound(m_Track.begin(), m_Track.end(), n); };
    Note* GetLastNoteAtTrack(int iTrackNum=-1, int iType=-1, int iSubType=-1);


    // @description calculate fBeat from iRow(after editing)
    void CalculateNoteBeat();
    // @description fill all note's timing data from beat data.
    void FillTimingData(TimingData& td);


    /*
     * metadata utilities
     */
    void FillSummaryData(ChartSummaryData& cmd);
    // @description 
    bool IsRangeEmpty(int iStartRow, int iEndRow);
    bool IsRowEmpty(int row);
    bool IsTrackEmpty(int track);
    bool IsEmpty();


    /*
     * modification utilities
     */
    bool IsHoldNoteAtRow(int row, int track = -1);
    NoteType GetNoteTypeAtRow(int row, int track = -1);
    Note* GetNoteAtRow(int row, int track=-1);
    int GetNoteIndexAtRow(int row, int track = -1);
    void GetNotesWithType(NoteSelection &vNotes, int nType=-1, int subType=-1);
    void GetNotesAtRow(NoteSelection &vNotes, int row = -1, int track = -1);
    void RemoveNotes(int iStartRow, int iEndRow, bool bInclusive);
    void RemoveNotes(const NoteSelection& vNotes);
    void CopyRange(int rowFromBegin, int rowFromLength, int rowToBegin);
    void CopyRange(const NoteSelection& vNotes, int rowToBegin);
    void MoveRange(int rowFromBegin, int rowFromLength, int rowToBegin);
    void MoveRange(const NoteSelection& vNotes, int rowToBegin);
    void InsertBlank(int rowFromBegin, int rowFromLength);
    void SetNoteDuplicatable(int bNoteDuplicatable);
    void AddNote(const Note& n);
    void AddTapNote(int iRow);
    void AddLongNote(int iRow, int iLength);
    

    /*
     * modification(option) utilities
     */
    void TrackMapping(int *trackmap, int s, int e);
    // @description useful for iidx(DP) style
    void TrackRandom(int side, int key);
    void TrackSRandom(int side, int key, bool bHrandom=false);
    void TrackRRandom(int side, int key);
    void TrackMirror(int side, int key);
    void TrackAllSC(int side);
    void TrackFlip();
    

    NoteData();
    ~NoteData();
    std::string const toString();

    // don't call these methods directly; use it from Chart.h
    void SetResolution(int iRes);
    void UpdateBeatData();
private:
    // @description All Objects are placed in here.
    // sorted in Row->x->y
    Track m_Track;
    // @description
    // Resolution of NoteData's Row position
    // Should have same value with Timingdata.
    int m_iRes;
    // @description
    // allow duplication in track based game? (iRow and track duplicatable)
    int m_bNoteDuplicatable;
};

class NoteSelection
{
private:
    // COMMENT: this vector should be in always sorted state
    std::vector<Note*> m_vNotes;
    friend class NoteData;      // only notedata class can directly access to this object
public:
    void SelectNote(Note* n);
    void UnSelectNote(const Note* n);
    std::vector<Note*>& GetSelection();
    std::vector<Note*>::iterator begin();
    std::vector<Note*>::iterator end();
    void Clear();
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