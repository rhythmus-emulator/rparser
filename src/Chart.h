/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTDATA_H
#define RPARSER_CHARTDATA_H

#include "common.h"
#include "MetaData.h"

namespace rparser
{

namespace chart
{
/*
 * structure of trackno (32bit)
 * 8bit: channel type
 * 8bit: channel subtype
 * 16bit: duplicated index (used for BGA channel / duplicated note, pos)
 */
typedef uint32_t rowid_t;
typedef struct track_s
{
  uint8_t type;
  uint8_t subtype;
  union {
    uint16_t col;
    struct { uint8_t x,y; } pos;
  } idx;
} track_t;

/*
 * @description
 * Soundable note such as sound, length, etc. (except for position info)
 */
struct Note
{
  rowid_t beat;     // position of row
  rowid_t length;   // length of note in row (if LN with specified length exists)
  track_t track;    // track information
  int value;        // command value
  int value_end;    // command value (for longnote in bmson)

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

/*
 * @description
 * Special objects whose position is based on beat.
 */
enum class TEMPOCMDTYPE
{
  EMPTY,
  BPM,
  STOP,
  WARP,
  SCROLL,
  MEASURE,
  TICK
};

struct Tempo
{
  TEMPOCMDTYPE type;
  uint32_t time;      // object position in time (msec)
  union {
    float d;
    uint32_t i;
  } value;
};

struct TempoByBeat
{
  TEMPOCMDTYPE type;
  double beat;      // object position in beat
  union {
    float d;
    uint32_t i;
  } value;
};

/** @detail Soundable/Renderable, or tappable object. */
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

/** @detail Subtype of TRACK_TAP / TOUCH / VEFX */
enum class SUBTYPE_NOTE
{
  NOTE_EMPTY,
  NOTE_TAP,       // general tappable / scorable note
  NOTE_INVISIBLE, // invisible and no damage, but scorable. changes keysound (bms)
  NOTE_LONGNOTE,  // general longnote.
  NOTE_CHARGE,    // general chargenote. (keyup at end point)
  NOTE_MINE,      // shock / mine note.
  NOTE_AUTOPLAY,  // drawn and sound but not judged,
  NOTE_FAKE,      // drawn but not judged nor sound.
  NOTE_COMBOZONE, // free combo area (Taigo yellow note / DJMAX stick rotating)
  NOTE_DRAG,      // dragging longnote (TECHNICA / deemo / SDVX VEFX ...)
  NOTE_CHAIN,     // chain longnote (TECHNICA / osu)
  NOTE_REPEAT,    // repeat longnote (TECHNICA)
};

/** @detail XXX: should fix */
enum class SUBTYPE_SOUND
{
  SOUND_NONE,
  SOUND_WAV,
  SOUND_MIDI,
};

/** @detail Subtype of TRACK_BGA */
enum class SUBTYPE_BGA
{
  BGA_EMPTY,
  BGA_BASE,
  BGA_MISS,
  BGA_LAYER1,
  BGA_LAYER2, /* on and on ... */
};

/** @detail Subtype of TRACK_SPECIAL */
enum class SUBTYPE_SPECIAL
{
  SPC_EMPTY,
  SPC_REST,           // osu specific type (REST area)
  SPC_KEYBIND,        // key bind layer (bms #SWBGA)
  SPC_BMS_EXTCHR,     // #EXTCHR cmd from BMS. (not supported)
  SPC_BMS_STOP,       // #STOP command from metadata 
  SPC_BMS_BPM,        // #BPM / #EXBPM command from metadata 
  SPC_BMS_TEXT,       // #TEXT / #SONG command in BMS
  SPC_BMS_OPTION,     // #CHANGEOPTION command in BMS
  SPC_BMS_ARGB_BASE,  // BGA opacity setting (#ARGB channel)
  SPC_BMS_ARGB_MISS,  // BGA opacity setting (#ARGB channel)
  SPC_BMS_ARGB_LAYER1,// BGA opacity setting (#ARGB channel)
  SPC_BMS_ARGB_LAYER2,// BGA opacity setting (#ARGB channel)
};

/*
 * @detail
 * Special objects which are queued in time.
 * (currently undefined object)
 */
enum class ACTIONTYPE
{
  TYPE_EMPTY,
};

struct Action
{
  ACTIONTYPE type;
  unsigned int time_ms;
  unsigned int value;
};

typedef std::vector<Tempo> TempoData;
typedef std::vector<Action> ActionData;
typedef std::vector<Note> NoteData;

class ConditionStatement;

/*
 * \detail
 * Contains all object consisting chart in primitive form, without calculating time and sorting.
 *
 * \params
 * metadata_ metadata to be used (might be shared with other charts)
 * timingdata_ timingdata to be used (might be shared with other charts)
 */
class Chart
{
/* iterators */
public:
  Chart(const MetaData *md, const TempoData *td);
  Chart(const Chart &nd);
  ~Chart();

  NoteData& GetNoteData();
  ActionData& GetActionData();
  TempoData& GetTempoData();
  MetaData& GetMetaData();
  const NoteData& GetNoteData() const;
  const ActionData& GetActionData() const;
  const TempoData& GetTempoData() const;
  const MetaData& GetMetaData() const;
  const TempoData& GetSharedTempoData() const;
  const MetaData& GetSharedMetaData() const;

  void Clear();
  // copy and merge objects from other ChartData (without stmt)
  void Merge(const Chart &cd, rowid_t rowFrom = 0);

  void AppendStmt(ConditionStatement& stmt);
  // evaluate all stmt and generate objects (process control flow)
  void EvaluateStmt(int seed = -1);

  void swap(Chart& c);

  virtual std::string toString() const;

  // others
  bool IsEmpty();

private:
  const TempoData* tempodata_shared_;
  const MetaData* metadata_shared_;
  NoteData notedata_;
  TempoData tempodata_;
  MetaData metadata_;
  ActionData actiondata_;
  std::vector<ConditionStatement> stmtdata_;
};

/*
 * \detail
 * Conditional Flow Statements
 */
class ConditionStatement
{
public:
  void AddSentence(unsigned int cond, Chart* chartdata);
  Chart* EvaluateSentence(int seed = -1);

  ConditionStatement(int value, bool israndom, bool isswitchstmt);
  ConditionStatement(ConditionStatement& cs);
  ~ConditionStatement();
private:
  int value_;
  bool israndom_;
  bool isswitchstmt_;
  std::map<int, Chart*> sentences_;
};

} /* namespace chart */

} /* namespace rparser */

#endif
