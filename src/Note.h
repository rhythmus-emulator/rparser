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
  Time,
  Beat,
  Bar
};

struct NotePos
{
  NotePos();
  NotePos(const NotePos&) = default;
  NotePos& operator=(const NotePos &) = default;

  NotePosTypes type;
  double time_msec;
  double beat;
  double measure;
  uint32_t denominator;

  void SetRowPos(uint32_t measure, RowPos deno, RowPos num);
  void SetRowPos(double barpos);
  void SetBeatPos(double beat);
  void SetTimePos(double time_msec);
  void SetDenominator(uint32_t denominator);
  NotePos& pos();
  const NotePos& pos() const;
  NotePosTypes postype() const;
  double GetBeatPos() const;
  double GetTimePos() const;

  bool operator==(const NotePos &other) const noexcept;
  bool operator<(const NotePos &other) const noexcept;
  std::string toString() const;
};

typedef struct
{
  union {
    struct { uint8_t player, lane; } note;
    struct { uint8_t x, y; } touch;
  } lane;
} NoteTrack;


/** @detail Soundable/Renderable, or tappable object. */
enum NoteTypes
{
  kNone,
  kTap,           // Tappable note object with column position.
  kTouch,         // Touch note object with x/y coordination.
  kBGM,           // autoplay & indrawable background sound object.
  kTempo,         // Tempo related objects
  kEvent,         // Other event triggering objects (e.g. BGA, MIDI)
};

/** @detail Subtype of TRACK_TAP / TOUCH / VEFX */
enum NoteSubTypes
{
  kNormalNote,    // general tappable / scorable note
  kInvisibleNote, // invisible and no damage, but scorable. changes keysound (bms)
  kLongNote,      // bms-type longnote with start time judging only. (BMS / TECHNICA / osu)
  kChargeNote,    // general chargenote. (consider keyup judge)
  kHChargeNote,   // hell chargenote (continous tick)
  kMineNote,      // shock / mine note.
  kAutoNote,      // drawn and sound but not judged,
  kFakeNote,      // drawn but not judged nor sound.
  kComboNote,     // free combo area (Taigo yellow note / DJMAX stick rotating)
  kDragNote,      // dragging longnote, check key_is_pressed input only. (TECHNICA / deemo / SDVX VEFX ...)
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

/** @detail Subtype of TRACK_BGA (for BMS type) */
enum BgaTypes
{
  kBgaMain,
  kBgaMiss,
  kBgaLAYER1,
  kBgaLAYER2, /* on and on ... */
};

/** @detail Subtype of TRACK_SPECIAL */
enum NoteEventTypes
{
  kBGA,
  kMIDI,
  kBmsKeyBind,      // key bind layer (bms #SWBGA)
  kBmsEXTCHR,       // #EXTCHR cmd from BMS. (not supported)
  kBmsTEXT,         // #TEXT / #SONG command in BMS
  kBmsBMSOPTION,    // #CHANGEOPTION command in BMS
  kBmsARGBLAYER,    // BGA opacity setting (#ARGB channel)
};

/**
 * @detail Contains base information of a Note, includes type and position data.
 */
class Note : public NotePos
{
public:
  Note() = default;
  Note(const Note&) = default;
  Note& operator=(const Note &) = default;

  NoteType type() const;
  NoteType subtype() const;
  void set_type(NoteType t);
  void set_subtype(NoteType t);

  bool operator==(const Note &other) const noexcept;
  std::string toString() const;
private:
  virtual std::string getValueAsString() const = 0;
  NoteType type_;
  NoteType subtype_;
};

struct NoteChain;

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
  SoundNote(const SoundNote &) = default;
  SoundNote& operator=(const SoundNote &) = default;
  SoundNote(SoundNote &&note) noexcept = default; // move constructor for vector resize

  void SetAsBGM(uint8_t col);
  void SetAsTouchNote();
  void SetAsTapNote(uint8_t player, uint8_t lane);
  void SetAsChainNote();
  void SetMidiNote(float duration_ms, int32_t key, float volume = 1.0f);
  void SetLongnoteLength(double delta_value);
  void SetLongnoteEndPos(const NotePos& row_pos);
  void SetLongnoteEndValue(Channel v);
  void AddChain(const NotePos& pos, uint8_t col);
  void AddTouch(const NotePos& pos, uint8_t x, uint8_t y);
  void ClearChain();
  bool IsLongnote() const;
  bool IsScoreable() const;

  uint8_t GetPlayer();
  uint8_t GetLane();
  uint8_t GetBGMCol();
  uint8_t GetX();
  uint8_t GetY();

  // basic note attributes
  NoteTrack track;
  // track / channel of sound source.
  Channel value;
  // indicates channel source type : WAVE or MIDI (or somewhat other?)
  int channel_type;
  
  // midi related properties (or somewhat sound effector)
  struct {
    // sound wave duration
    float duration_ms;
    // pitch diff or base pitch of sound
    int32_t key;
    // volume of sound (default 1.0f)
    float volume;
    // bmson attribute. (loop)
    bool restart;
  } effect;

  // longnote / chain data
  // main reason why SoundNote object requires move constructor.
  std::vector<NoteChain> chains;

  bool operator==(const SoundNote &other) const noexcept;
private:
  virtual std::string getValueAsString() const;
};

struct NoteChain
{
  NoteTrack track;
  NotePos pos;
  Channel value;
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

/**
 * @detail
 * Other event related objects (not tempo / sound related; BGA, ...)
 */
class EventNote : public Note
{
public:
  EventNote();
  EventNote(const EventNote&) = default;

  void SetBga(BgaTypes bgatype, Channel channel, uint8_t column = 0);
  void SetMidiCommand(uint8_t command, uint8_t arg1, uint8_t arg2 = 0);
  void SetBmsARGBCommand(BgaTypes bgatype, Channel channel);

  void GetMidiCommand(uint8_t &command, uint8_t &arg1, uint8_t &arg2);

  bool operator==(const EventNote &other) const noexcept;
private:
  int32_t command_, arg1_, arg2_;
  virtual std::string getValueAsString() const;
};


template <typename N>
class NoteData
{
public:
  NoteData(): is_sorted_(false) {};
  NoteData(const NoteData& notedata)
    : notes_(notedata.notes_), is_sorted_(notedata.is_sorted_) { }
  void swap(NoteData& notedata) noexcept
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
  void AddNote(const N &n) noexcept
  {
    if (is_sorted_)
    {
      auto it = std::upper_bound(begin(), end(), n);
      notes_.insert(it, n);
    }
    else notes_.push_back(n);
  }
  void AddNote(N &&n) noexcept
  {
    if (is_sorted_)
    {
      auto it = std::upper_bound(begin(), end(), n);
      notes_.emplace(it, n);
    }
    else notes_.emplace_back(n);
  }
  size_t size() const { return notes_.size(); }
  N& front() { return notes_.front(); }
  N& back() { return notes_.back(); }
  const N& back() const { return notes_.back(); }
  const N& front() const { return notes_.front(); }
  N& get(size_t i) { return notes_[i]; }
  const N& get(size_t i) const { return notes_[i]; }
  typename std::vector<N>::iterator begin() { return notes_.begin(); }
  typename std::vector<N>::iterator end() { return notes_.end(); }
  const typename std::vector<N>::const_iterator begin() const { return notes_.begin(); }
  const typename std::vector<N>::const_iterator end() const { return notes_.end(); }
  typename std::vector<N>::reverse_iterator rbegin() { return notes_.rbegin(); }
  typename std::vector<N>::reverse_iterator rend() { return notes_.rend(); }
  const typename std::vector<N>::const_reverse_iterator rbegin() const { return notes_.rbegin(); }
  const typename std::vector<N>::const_reverse_iterator rend() const { return notes_.rend(); }
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
