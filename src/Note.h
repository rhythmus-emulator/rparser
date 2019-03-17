#ifndef RPARSER_NOTE_H
#define RPARSER_NOTE_H

#include "common.h"

namespace rparser
{

typedef uint16_t RowPos;
typedef uint8_t NoteType;
typedef uint32_t Channel;

enum class NotePosTypes
{
  NullPos,
  Row,
  Time,
  Beat
};

struct Row
{
  uint32_t measure;
  RowPos num;
  RowPos deno;
};

struct NotePos
{
  NotePosTypes type;
  Row row;
  double time_msec;
  double beat;

  bool operator==(const NotePos &other) const;
};

typedef struct
{
  union {
    struct { uint8_t player, lane; } note;
    struct { uint8_t x, y; } touch;
  } lane;
} NoteTrack;

/**
 * @detail Contains base information of a Note, includes type and position data.
 */
class Note
{
public:
  NoteType type;
  NoteType subtype;
  NotePos pos;
  Note *prev;
  Note *next;

  bool operator<(const Note &other) const;
  bool operator==(const Note &other) const;
  std::string toString() const;
};

/**
 * @detail
 * Soundable note object (Bgm / Tappable)
 *
 * @param
 * prev previous chained object (null when normal note)
 * next next chained object (null when normal note)
 * value
 *   value : general value
 *   sound : type (wav or midi), channel (which channel)
 */
class SoundNote : public Note
{
public:
  NoteTrack track;
  Channel value;
  float volume;
  int32_t pitch;
  // @description bmson attribute. (loop)
  bool restart;
};

/**
 * @detail
 * Bga note object
 */
class BgaNote : public Note
{
public:
  Channel value;
};

/**
 * @detail
 * Tempo note object
 */
class TempoNote : public Note
{
public:
  struct
  {
    float f;
    int32_t i;
  } value;
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

template <typename N>
class NoteData
{
public:
  NoteData(): is_sorted_(false) {};
  NoteData(const NoteData& notedata)
    : notes_(notedata.notes_), is_sorted_(notedata.is_sorted_) { }
  void swap(NoteData& notedata)
  {
    std::swap(notes_, notedata.notes_);
    std::swap(is_sorted_, notedata.is_sorted_);
  }
  void clear()
  {
    notes_.clear();
    is_sorted_ = false;
  }
  void Merge(const NoteData& notedata, RowPos rowstart = 0)
  {
    // TODO: implement rowFrom
    notes_.insert(notes_.end(), notedata.begin(), notedata.end());
  }
  void Cut(double start_beat, double end_beat) { /* TODO */ }
  void SortByBeat()
  {
    if (is_sorted_) return;
    std::sort(begin(), end());
    is_sorted_ = true;
  }
  void SortModeOff() { is_sorted_ = false; }
  bool IsSorted() { return is_sorted_; }
  void AddNote(const N &n)
  {
    if (is_sorted_)
    {
      auto it = std::upper_bound(begin(), end(), n);
      notes_.insert(it, n);
    }
    else notes_.push_back(n);
  }
  void AddNote(N &&n)
  {
    if (is_sorted_)
    {
      auto it = std::upper_bound(begin(), end(), n);
      notes_.insert(it, n);
    }
    else notes_.push_back(n);
  }
  typename std::vector<N>::iterator begin() { return notes_.begin(); }
  typename std::vector<N>::iterator end() { return notes_.end(); }
  typename const std::vector<N>::const_iterator begin() const { return notes_.begin(); }
  typename const std::vector<N>::const_iterator end() const { return notes_.end(); }
  bool IsEmpty() const { return notes_.size(); }
  std::string toString() const
  {
    std::stringstream ss;
    ss << "Total note count: " << notes_.size() << std::endl;
    ss << "Is sorted? " << is_sorted_ << std::endl;
    return ss.str();
  }
private:
  std::vector<N> notes_;
  bool is_sorted_;
};

}

#endif