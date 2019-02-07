/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTDATA_H
#define RPARSER_CHARTDATA_H

#include "MetaData.h"
#include <map>
#include <vector>
#include <algorithm>

/*
 * NOTE:
 * ChartData stores note object in semantic objects, not raw object.
 * These modules written below does not contain status about current modification.
 */


namespace rparser
{

/*
 * @description
 * Soundable/Renderable, or tappable object.
 */
enum class TRACKTYPE
{
    TRACK_EMPTY,
    TRACK_TAP,           // Simple note object (key input)
	TRACK_TOUCH,         // Osu like object (touch input)
	TRACK_VEFX,          // SDVX like object (level input)
	TRACK_BGM,           // autoplay & indrawable background sound object.
	TRACK_BGA,           // drawable object
	TRACK_SPECIAL,       // special objects (mostly control flow)
};

// These types are also appliable to TOUCH / VEFX type also.
// COMMENT: NoteTapType has lane information at 0xF0!
enum class NoteType
{
    NOTE_EMPTY,
    NOTE_TAP,			// general tappable / scorable note
    NOTE_INVISIBLE,		// invisible and no damage, but scorable. changes keysound (bms)
	NOTE_LONGNOTE,		// general longnote.
	NOTE_CHARGE,		// general chargenote. (keyup at end point)
    NOTE_MINE,			// shock / mine note.
    NOTE_AUTOPLAY,		// drawn and sound but not judged,
    NOTE_FAKE,			// drawn but not judged nor sound.
    NOTE_COMBOZONE,		// free combo area (Taigo yellow note / DJMAX stick rotating)
	NOTE_DRAG,			// dragging longnote (TECHNICA / deemo / SDVX VEFX ...)
	NOTE_CHAIN,			// chain longnote (TECHNICA / osu)
	NOTE_REPEAT,		// repeat longnote (TECHNICA)
};

enum class SoundType
{
	SOUND_NONE,
	SOUND_WAV,
	SOUND_MIDI,
};

enum class BgaType
{
    BGA_EMPTY,
    BGA_BASE,
    BGA_MISS,
	BGA_LAYER1,
	BGA_LAYER2, /* on and on ... */
};

enum class TrackSpecialType
{
	SPC_EMPTY,
	SPC_REST,			// osu specific type (REST area)
	SPC_KEYBIND,		// key bind layer (bms #SWBGA)
	SPC_BMS_EXTCHR,		// #EXTCHR cmd from BMS. (not supported)
	SPC_BMS_STOP,		// #STOP command from metadata 
	SPC_BMS_BPM,		// #BPM / #EXBPM command from metadata 
	SPC_BMS_TEXT,		// #TEXT / #SONG command in BMS
	SPC_BMS_OPTION,		// #CHANGEOPTION command in BMS
	SPC_BMS_ARGB_BASE,	// BGA opacity setting (#ARGB channel)
	SPC_BMS_ARGB_MISS,	// BGA opacity setting (#ARGB channel)
	SPC_BMS_ARGB_LAYER1,// BGA opacity setting (#ARGB channel)
	SPC_BMS_ARGB_LAYER2,// BGA opacity setting (#ARGB channel)
};

/*
 * structure of trackno (64bit)
 * 8bit: duplicated index (used for BGA channel / duplicated note)
 * 8bit: channel subtype
 * 8bit: channel type
 * 32bit: row number
 */
typedef unsigned long long rowid;
typedef struct trackinfo
{
	uint8_t idx;
	uint8_t lane;
	uint8_t subtype;
	uint8_t type;
	uint32_t row;

	trackinfo() : idx(0), lane(0), subtype(0), type(0), row(0) {}
};

/*
 * @description
 * Note data such as sound, length, etc. (except for position info)
 */
struct NoteData
{
	rowid length;					// length of note in row (if LN with specified length exists)
    int value;                      // command value
	int value_end;					// command value (for longnote)
	int x, y;                       // (in case of TOUCH object)

    float volume;
    int pitch;

    // @description bmson attribute. (loop)
    bool restart;

	NoteData() : value(0), x(0), y(0),
		volume(0), pitch(0), restart(false) {}

    std::string toString();
	bool operator<(const NoteData &other) const;
	bool operator==(const NoteData &other) const;
    bool IsTappableNote();
    int GetPlayerSide();
    int GetTrack();
};
typedef std::pair<trackinfo, NoteData> Note;

/*
 * @description
 * Special objects whose position is based on beat.
 */
enum class TIMINGOBJTYPE
{
	TYPE_NONE,
	TYPE_BPM,
	TYPE_STOP,
	TYPE_WARP,
	TYPE_SCROLL,
	TYPE_MEASURE,
	TYPE_TICK,
	TYPE_INVALID
};

struct TimingObject
{
	TIMINGOBJTYPE type;
	double beat;			// object position in beat
	double value;			// detail desc of timing object.
};



/*
 * @description
 * Special objects which are queued in time.
 * (currently undefined object)
 */
enum class ACTIONTYPE
{
	TYPE_NONE,
};

struct Action
{
	ACTIONTYPE type;
	unsigned int time_ms;
	unsigned int value;
};



/*
 * @description
 * Conditional Flow Statement
 */
class ConditionStatement
{
public:
	void AddSentence(unsigned int cond, ChartData* chartdata);
	ChartData* EvaluateSentence(int seed = -1);

	ConditionStatement(int value, bool israndom, bool isswitchstmt);
	~ConditionStatement();
private:
	int value_;
	bool israndom_;
	bool isswitchstmt_;
	std::map<unsigned int, ChartData*> sentences_;
};


class NoteSelection;


/*
 * @description
 * ChartData consists with track / timingdata / metadatas, parsed for each section.
 */
class ChartData
{
/* iterators */
public:
    typedef std::vector<Note>::iterator noteiter;
    typedef std::vector<Note>::const_iterator const_noteiter;
	noteiter begin() { return notes_.begin(); };
	noteiter end() { return notes_.end(); };
	const_noteiter begin() const { return notes_.begin(); };
	const_noteiter end() const { return notes_.end(); };
	noteiter lower_bound(int row);
	noteiter upper_bound(int row);
	noteiter lower_bound(Note &n) { return std::lower_bound(notes_.begin(), notes_.end(), n); };
	noteiter upper_bound(Note &n) { return std::upper_bound(notes_.begin(), notes_.end(), n); };


	typedef std::vector<TimingObject>::iterator tobjiter;
	typedef std::vector<TimingObject>::const_iterator const_tobjiter;
	tobjiter tobj_begin() { return timingobjs_.begin(); };
	tobjiter tobj_end() { return timingobjs_.end(); };
	const_tobjiter tobj_begin() const { return timingobjs_.begin(); };
	const_tobjiter tobj_end() const { return timingobjs_.end(); };
	tobjiter tobj_lower_bound(int row);
	tobjiter tobj_upper_bound(int row);
	tobjiter tobj_lower_bound(Note &n) { return std::lower_bound(timingobjs_.begin(), timingobjs_.end(), n); };
	tobjiter tobj_upper_bound(Note &n) { return std::upper_bound(timingobjs_.begin(), timingobjs_.end(), n); };


	typedef std::vector<Action>::iterator actiter;
	typedef std::vector<Action>::const_iterator const_actiter;
	actiter act_begin() { return actions_.begin(); };
	actiter act_end() { return actions_.end(); };
	const_actiter act_begin() const { return actions_.begin(); };
	const_actiter act_end() const { return actions_.end(); };
	actiter act_lower_bound(int row);
	actiter act_upper_bound(int row);
	actiter act_lower_bound(Note &n) { return std::lower_bound(actions_.begin(), actions_.end(), n); };
	actiter act_upper_bound(Note &n) { return std::upper_bound(actions_.begin(), actions_.end(), n); };


	const std::vector<Note>* GetNotes() const;
	const std::vector<TimingObject>* GetTimingObjects() const;
	const std::vector<Action>* GetActions() const;
	const MetaData* GetMetaData() const;
	MetaData* GetMetaData();


	// Flush objects at once and sort it rather using AddXX() method for each of them.
	// NOTE: original vector object will be swapped(std::move).
	void Flush(std::vector<Note>& notes,
		std::vector<TimingObject>& tobjs,
		std::vector<Action>& actions);
	// clear all objects. (optional: case statements)
	void Clear(bool removeStmt = true);
	// copy and merge objects from other ChartData (without stmt)
	void Merge(const ChartData &cd, bool overwrite = true, rowid rowFrom = 0);
	// evaluate all stmt and generate objects (process control flow)
	void EvaluateStmt();


public:
	// row scanning utilities (fast-scan)
    bool IsRangeEmpty(unsigned long long startrow, unsigned long long endrow);
	bool IsRowEmpty(unsigned long long row, TRACKTYPE type = TRACKTYPE::TRACK_EMPTY, unsigned int subtype = 0);
	bool IsTrackEmpty(trackinfo track);
	bool IsHoldNoteAtRow(int row, int track = -1);
	Note* GetNoteAtRow(int row, int track = -1);
	void GetNotesAtRow(NoteSelection &vNotes, int row = -1, int track = -1);
	void GetNotesAtRange(NoteSelection &vNotes, int row = -1, int track = -1);


	// full scan utilities
	bool IsTypeEmpty(TRACKTYPE type, unsigned int subtype = 0);
	bool IsLaneEmpty(unsigned int lane);
	void GetNotesWithType(NoteSelection &vNotes, int nType = -1, int subType = -1);


	// others
    bool IsEmpty();


// insert / delete / update
public:
    void RemoveNotes(rowid iStartRow, rowid iEndRow, bool bInclusive);
    void RemoveNotes(const NoteSelection& vNotes);
    void CopyRange(rowid rowFromBegin, rowid rowFromLength, rowid rowToBegin, bool overwrite = true);
    void CopyRange(const NoteSelection& vNotes, rowid rowToBegin, bool overwrite = true);
	void MoveRange(rowid rowFromBegin, rowid rowFromLength, rowid rowToBegin);
	void MoveRange(const NoteSelection& vNotes, rowid rowToBegin);
	void ShiftLane(rowid rowFromBegin, rowid rowFromLength, unsigned int newLane);
	void ShiftLane(const NoteSelection& vNotes, unsigned int newLane);
	void ShiftType(rowid rowFromBegin, rowid rowFromLength, TRACKTYPE newType, unsigned int newSubType);
	void ShiftType(const NoteSelection& vNotes, TRACKTYPE newType, unsigned int newSubType);
    void InsertBlank(rowid rowFromBegin, rowid rowFromLength);
    void AddNote(const Note& n, bool overwrite = true);
    void AddTapNote(rowid iRow, unsigned int lane);
    void AddLongNote(rowid iRow, unsigned int lane, rowid iLength);


	void AddTimingObject(const TimingObject& tobj);
	void RemoveTimingObject(rowid start, rowid end);
	void AddAction(const TimingObject& tobj);
	void RemoveAction(rowid start, rowid end);
    

// modification(option) utilities
public:
    void LaneMapping(int *trackmap, int s, int e);
    // @description useful for iidx(DP) style
    void LaneRandom(int side, int key);
    void LaneSRandom(int side, int key, bool bHrandom=false);
    void LaneRRandom(int side, int key);
    void LaneMirror(int side, int key);
    void LaneAllSC(int side);
    void LaneFlip();
    

public:
	ChartData();
	ChartData(ChartData &nd);
    ~ChartData();
    std::string const toString();


private:
	/*
	 * Contains all note objects
	 * (ROW based position; mostly exists lane/channel and not duplicable.)
	 * Includes not only sound/rendering but also special/timing object.
	 * note of same idx + subtype + type + row should be overwritten.
	 */
	std::vector<Note> notes_;
	/*
	 * Contains all timing related objects
	 * (BEAT based position; mostly no lane/channel and duplicable.)
	 * NOTE: do not need to attempt render TimingObject using this data ...
	 *       Row position is already calculated at TimingData.
	 */
	std::vector<TimingObject> timingobjs_;
	/*
	 * Contains all time-reserved objects
	 * (TIME based objects; mostly used for special action [undefined])
	 */
	std::vector<Action> actions_;
	/*
	 * Metadata of this chart section.
	 */
	MetaData metadata_;
	/*
	 * Conditional flow
	 */
	std::vector<ConditionStatement> stmts_;
};

class NoteSelection
{
private:
    // COMMENT: this vector should be in always sorted state
    std::vector<Note*> m_vNotes;
    friend class ChartData;      // only chartdata class can directly access to this object
public:
    void SelectNote(Note* n);
    void UnSelectNote(const Note* n);
    std::vector<Note*>& GetSelection();
    std::vector<Note*>::iterator begin();
    std::vector<Note*>::iterator end();
    void Clear();
};


// Charge note type for mixing object
enum class CNTYPE
{
	NONE = 0,
	BMS,		// BMS type (no judge at the end of the CN)
	CN,			// IIDX charge note type
	HCN,		// IIDX hell charge note type
};

/*
 * @description
 * Soundable(Renderable) object in MixingData.
 * State is not stored; should be generated by oneself if necessary.
 */
struct MixingNote
{
	// mixing related

	bool ismixable;
	unsigned int cmd;
	unsigned int cmdarg;
	unsigned int value_start;
	unsigned int value_end;
	double time_start_ms;
	double time_end_ms;
	bool loop;

	// playing related

	bool isplayable;
	CNTYPE cntype;
	unsigned int lane;
	unsigned long long row_start;
	unsigned long long row_end;

	// mouse motion related (osu!)

	void* mouse_ptrs[64];
	int mouse_ptr_len;

	// reference pointer to original Note object

	Note* obj_start;
	Note* obj_end;
};



}

#endif
