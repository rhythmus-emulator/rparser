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
  Row() = default;
  Row(uint32_t measure, RowPos num, RowPos deno);

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

  bool operator==(const NotePos &other) const noexcept;
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
  Note() = default;
  Note(const Note&) = default;

  NoteType GetNotetype() const;
  NoteType GetNoteSubtype() const;
  void SetNotetype(NoteType t);
  void SetNoteSubtype(NoteType t);
  void SetRowPos(const Row& r);
  void SetRowPos(uint32_t measure, RowPos deno, RowPos num);
  void SetBeatPos(double beat);
  void SetTimePos(double time_msec);
  NotePos& GetNotePos();
  const NotePos& GetNotePos() const;
  NotePosTypes GetNotePosType() const;

  bool operator<(const Note &other) const noexcept;
  bool operator==(const Note &other) const noexcept;
  std::string toString() const;

  Note *prev;
  Note *next;
private:
  virtual std::string getValueAsString() const = 0;
  NotePos pos;
  NoteType type;
  NoteType subtype;
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
  SoundNote();
  void SetAsBGM();
  void SetAsTouchNote();
  void SetAsTapNote();
  void SetAsKnobNote();

  NoteTrack track;
  Channel value;
  float volume;
  int32_t pitch;
  // @description bmson attribute. (loop)
  bool restart;
  bool operator==(const SoundNote &other) const noexcept;
private:
  virtual std::string getValueAsString() const;
};

/**
 * @detail
 * Bga note object
 */
class BgaNote : public Note
{
public:
  BgaNote();

  Channel value;
  bool operator==(const BgaNote &other) const noexcept;
private:
  virtual std::string getValueAsString() const;
};

/**
 * @detail
 * Tempo note object
 */
class TempoNote : public Note
{
public:
  TempoNote();

  void SetBpm(float bpm);
  void SetBmsBpm(Channel bms_channel);
  void SetStop(float stop);
  void SetBmsStop(Channel bms_channel);
  void SetMeasure(float measure_length);
  void SetScroll(float scrollspeed);
  void SetTick(int32_t tick);
  void SetWarp(float warp_to);
  float GetFloatValue() const;
  int32_t GetIntValue() const;

  bool operator==(const TempoNote &other) const noexcept;
private:
  union
  {
    float f;
    int32_t i;
  } value;
  virtual std::string getValueAsString() const;
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
  const typename std::vector<N>::const_iterator begin() const { return notes_.begin(); }
  const typename std::vector<N>::const_iterator end() const { return notes_.end(); }
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
