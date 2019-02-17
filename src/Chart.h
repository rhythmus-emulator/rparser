/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTDATA_H
#define RPARSER_CHARTDATA_H

#include "common.h"
#include "MetaData.h"

namespace rparser
{

/*
 * structure of trackno (32bit)
 * 8bit: channel type
 * 8bit: channel subtype
 * 16bit: duplicated index (used for BGA channel / duplicated note, pos)
 */
typedef uint16_t RowPos;
typedef uint8_t CObjType;

typedef struct
{
  CObjType type;
  CObjType subtype;
  union {
    struct { uint16_t col; } tap;
    struct { uint8_t x,y; } touch;
  } channel;
} ChartObjTrack;

enum class ChartObjPosType
{
  Row,
  Time,
  Beat
};

union ChartObjPos
{
  struct
  {
    uint32_t measure;
    RowPos num;
    RowPos deno;
  } Row;
  struct
  {
    double msec;
  } Time;
  struct
  {
    double beat;
  } Beat;
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
struct ChartObject
{
  ChartObjPosType pos_type;
  ChartObjPos pos;
  ChartObjPos pos_end;    // for MIDI or bmson type.
  ChartObjTrack track;
  ChartObject *prev;
  ChartObject *next;

  union {
    int32_t value;
    struct { int16_t type, channel; } sound;
  } value;

  float volume;
  int32_t pitch;
  // @description bmson attribute. (loop)
  bool restart;

  bool operator<(const ChartObject &other) const;
  bool operator==(const ChartObject &other) const;
};

/** @detail Soundable/Renderable, or tappable object. */
enum CObjType
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
enum CObjSubtypeNote
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
enum CObjTypeTempo
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

#if 0
/** @detail XXX: should fix */
enum class SUBTYPE_SOUND
{
  SOUND_NONE,
  SOUND_WAV,
  SOUND_MIDI,
};
#endif

/** @detail Subtype of TRACK_BGA */
enum CObjSubtypeBGA
{
  kMainBga,
  kMissBga,
  kLAYER1Bga,
  kLAYER2Bga, /* on and on ... */
};

/** @detail Subtype of TRACK_SPECIAL */
enum class CObjSubtypeSpecial
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

typedef std::vector<ChartObject> ChartData;

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
  Chart(const MetaData *md, const ChartData *td);
  Chart(const Chart &nd);
  ~Chart();

  ChartData& GetChartData();
  MetaData& GetMetaData();
  const ChartData& GetChartData() const;
  const MetaData& GetMetaData() const;
  const ChartData& GetSharedChartData() const;
  const MetaData& GetSharedMetaData() const;

  void Clear();
  // copy and merge objects from other ChartData (without stmt)
  void Merge(const Chart &cd, RowPos rowFrom = 0);

  void AppendStmt(ConditionStatement& stmt);
  // evaluate all stmt and generate objects (process control flow)
  void EvaluateStmt(int seed = -1);

  void swap(Chart& c);

  virtual std::string toString() const;

  // others
  bool IsEmpty();

private:
  const ChartData* chartdata_shared_;
  const MetaData* metadata_shared_;
  ChartData chartdata_;
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

} /* namespace rparser */

#endif
