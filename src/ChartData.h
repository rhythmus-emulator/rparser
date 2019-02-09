#pragma once

#include <stdint.h>
#include <map>
#include <vector>
#include <algorithm>

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


class TimingData
{

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