/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTDATA_H
#define RPARSER_CHARTDATA_H

#include "common.h"
#include "TempoData.h"
#include "MetaData.h"

namespace rparser
{

typedef uint16_t RowPos;
typedef uint8_t NoteType;
typedef uint32_t Channel;

typedef struct
{
  NoteType type;
  NoteType subtype;
  union {
    struct { uint8_t player,col; } note;
    struct { uint8_t x,y; } touch;
  } lane;
} NoteTrack;

enum class NotePosTypes
{
  Row,
  Time,
  Beat
};

struct NotePos
{
  NotePosTypes type;
  struct
  {
    uint32_t measure;
    RowPos num;
    RowPos deno;
  } row;
  double time_msec;
  double beat;

  bool operator==(const NotePos &other) const;
};

/*
 * @detail
 * All kinds of basic chart elements.
 * 
 * @param
 * prev previous chained object (null when normal note)
 * next next chained object (null when normal note)
 * value
 *   value : general value
 *   sound : type (wav or midi), channel (which channel)
 */
struct Note
{
  NotePos pos;
  NotePos pos_end;    // for MIDI or bmson type.
  NoteTrack track;
  Note *prev;
  Note *next;

  union {
    float f;
    int32_t i;
    Channel channel;
    struct { int16_t type, channel; } sound;
  } value;

  float volume;
  int32_t pitch;
  // @description bmson attribute. (loop)
  bool restart;

  bool operator<(const Note &other) const;
  bool operator==(const Note &other) const;
  std::string toString() const;
};

/** @detail Soundable/Renderable, or tappable object. */
enum NoteTypes
{
  kNone,
  kNote,          // Simple note object (key input)
  kTouch,         // Osu like object (touch input)
  kKnob,          // SDVX like object (level input)
  kBGM,           // autoplay & indrawable background sound object.
  kBGA,           // drawable object
  kTempo,         // Tempo related objects
  kSpecial,       // Special objects (mostly control flow)
};

/** @detail Subtype of TRACK_TAP / TOUCH / VEFX */
enum NoteSubTypes
{
  kNormalNote,    // general tappable / scorable note
  kInvisibleNote, // invisible and no damage, but scorable. changes keysound (bms)
  kLongNote,      // general longnote.
  kChargeNote,    // general chargenote. (keyup at end point)
  kHChargeNote,   // hell chargenote
  kMineNote,      // shock / mine note.
  kAutoNote,      // drawn and sound but not judged,
  kFakeNote,      // drawn but not judged nor sound.
  kComboNote,     // free combo area (Taigo yellow note / DJMAX stick rotating)
  kDragNote,      // dragging longnote (TECHNICA / deemo / SDVX VEFX ...)
  kChainNote,     // chain longnote (TECHNICA / osu)
  kRepeatNote,    // repeat longnote (TECHNICA)
};

/** @detail Special object which changes tempo of the chart. */
enum NoteTempoTypes
{
  kBpm,
  kStop,
  kWarp,
  kScroll,
  kMeasure,
  kTick,
  kBmsBpm,          // #BPM / #EXBPM command from metadata 
  kBmsStop,         // #STOP command from metadata
};

/** @detail Subtype of TRACK_BGA */
enum NoteBgaTypes
{
  kMainBga,
  kMissBga,
  kLAYER1Bga,
  kLAYER2Bga, /* on and on ... */
};

/** @detail Subtype of TRACK_SPECIAL */
enum NoteSpecialTypes
{
  kRest,            // osu specific type (REST area)
  kBmsKeyBind,      // key bind layer (bms #SWBGA)
  kBmsEXTCHR,       // #EXTCHR cmd from BMS. (not supported)
  kBmsTEXT,         // #TEXT / #SONG command in BMS
  kBmsBMSOPTION,    // #CHANGEOPTION command in BMS
  kBmsARGBBASE,     // BGA opacity setting (#ARGB channel)
  kBmsARGBMISS,     // BGA opacity setting (#ARGB channel)
  kBmsARGBLAYER1,   // BGA opacity setting (#ARGB channel)
  kBmsARGBLAYER2,   // BGA opacity setting (#ARGB channel)
};

struct NoteData
{
public:
  NoteData();
  NoteData(const NoteData& notedata);
  void swap(NoteData& notedata);
  void clear();
  void Merge(const NoteData& notedata, RowPos rowstart = 0);
  void SortByBeat();
  void SortModeOff();
  bool IsSorted();
  void AddNote(const Note &n);
  void AddNote(Note &&n);
  std::vector<Note>::iterator begin();
  std::vector<Note>::iterator end();
  const std::vector<Note>::const_iterator begin() const;
  const std::vector<Note>::const_iterator end() const;
private:
  std::vector<Note> notes_;
  bool is_sorted_;
};

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
  Chart(const MetaData *md, const NoteData *nd);
  Chart(const Chart &nd);
  ~Chart();

  NoteData& GetNoteData();
  MetaData& GetMetaData();
  const NoteData& GetNoteData() const;
  const MetaData& GetMetaData() const;
  const TempoData& GetTempoData() const;
  const NoteData& GetSharedNoteData() const;
  const MetaData& GetSharedMetaData() const;
  
  void Clear();

  void AppendStmt(ConditionStatement& stmt);
  // evaluate all stmt and generate objects (process control flow)
  void EvaluateStmt(int seed = -1);

  void swap(Chart& c);

  virtual std::string toString() const;

  void InvalidateAllNotePos();
  void InvalidateNotePos(Note &n);
  void InvalidateTempoData();

  // others
  bool IsEmpty();

private:
  const NoteData* notedata_shared_;
  const MetaData* metadata_shared_;
  NoteData notedata_;
  MetaData metadata_;
  TempoData tempodata_;
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
  Chart* EvaluateSentence(int seed = -1) const;

  ConditionStatement(int value, bool israndom, bool isswitchstmt);
  ConditionStatement(const ConditionStatement& cs);
  ~ConditionStatement();
private:
  mutable int value_;
  bool israndom_;
  bool isswitchstmt_;
  std::map<int, Chart*> sentences_;
};

} /* namespace rparser */

#endif
