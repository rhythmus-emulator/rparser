#ifndef RPARSER_NOTE_H
#define RPARSER_NOTE_H

#include "common.h"

namespace rparser
{

typedef uint16_t RowPos;
typedef uint8_t NoteType;
typedef uint32_t Channel;

/** @depreciated @brief Main method used to describe NotePos. */
enum class NotePosTypes
{
  NullPos,
  Time,
  Beat
};

/** @detail Subtype of TRACK_TAP / TOUCH / VEFX */
enum NoteTypes
{
  kNormalNote,      // general tappable / scorable note
  kInvisibleNote,   // invisible and no damage, but scorable. changes keysound (bms)
  kLongNote,        // bms-type longnote with start time judging only. (BMS / TECHNICA / osu)
  kChargeNote,      // general chargenote. (consider keyup judge)
  kHChargeNote,     // hell chargenote (continous tick)
  kMineNote,        // shock / mine note.
  kAutoNote,        // drawn and sound but not judged,
  kFakeNote,        // drawn but not judged nor sound.
  kComboNote,       // free combo area (Taigo yellow note / DJMAX stick rotating)
  kDragNote,        // dragging longnote, check key_is_pressed input only. (TECHNICA / deemo / SDVX VEFX ...)
  kRepeatNote,      // repeat longnote (TECHNICA)
};

/** @detail Subtype of TRACK_BGA (for BMS type) */
enum BgaTypes
{
  kBgaMiss,
  kBgaMain,
  kBgaLayer1,
  kBgaLayer2,
};

/** @detail Special object which changes tempo of the chart. */
enum TimingObjectTypes
{
  kMeasure,         // Bms type bar length
  kScroll,          // Stepmania type bar length
  kBpm,
  kStop,
  kWarp,
  kTick,
  kBmsBpm,          // #BPM / #EXBPM command from metadata 
  kBmsStop,         // #STOP command from metadata
};

/** @detail Subtype of TRACK_SPECIAL */
enum EffectObjectTypes
{
  kMIDI,
  kBmsKeyBind,      // key bind layer (bms #SWBGA)
  kBmsEXTCHR,       // #EXTCHR cmd from BMS. (not supported)
  kBmsTEXT,         // #TEXT / #SONG command in BMS
  kBmsBMSOPTION,    // #CHANGEOPTION command in BMS
  kBmsARGBLAYER,    // BGA opacity setting (#ARGB channel)
};


/**
 * @brief
 * Note position contains beat / time / measure information.
 * All position information is filled later by TimingData class.
 */
class NotePos
{
public:
  NotePos();
  NotePos(const NotePos&) = default;
  NotePos& operator=(const NotePos &) = default;

  double time_msec;
  double measure;
  RowPos num, deno;

  void SetRowPos(int measure, RowPos deno, RowPos num);
  void SetBeatPos(double beat);
  void SetTime(double time_msec);
  void SetDenominator(uint32_t denominator);
  NotePosTypes postype() const;
  double GetBeatPos() const;
  double GetTimePos() const;
  void set_track(int track);
  int get_track() const;
  virtual NotePos& endpos();
  const NotePos& endpos() const;

  bool operator==(const NotePos &other) const noexcept;
  bool operator<(const NotePos &other) const noexcept;
  std::string toString() const;
  virtual NotePos* clone() const;

private:
  NotePosTypes type;
  int track_;
};


/**
 * @brief
 * Description of position of tappable(scoreable) object.
 *
 * @param x, y, z position of note object.
 * @param tap_type required tapping type for this note.
 */
class NoteDesc : public NotePos
{
public:
  NoteDesc();
  void set_pos(int x, int y);
  void set_pos(int x, int y, int z);
  void get_pos(int &x, int &y) const;
  void get_pos(int &x, int &y, int &z) const;
  virtual NoteDesc* clone() const;

private:
  int x_, y_, z_;
};


/**
 * @brief
 * Description of note sound information.
 */
class NoteSoundDesc
{
public:
  NoteSoundDesc();

  void set_channel(Channel ch);
  void set_length(uint32_t len_ms);
  void set_volume(float v);
  void set_key(int key);
  Channel channel() const;
  float volume() const;
  uint32_t length() const;
  int key() const;
  virtual NoteSoundDesc* clone() const;

private:
  Channel channel_;
  uint32_t length_; /* in milisecond */
  float volume_; /* 0 ~ 1 */
  int key_;
};


/**
 * @brief
 * Scorable element.
 * If combo breaks in chain, then next element of the chain is inavailable.
 * Therefore, need to enter full chain correctly to get full score.
 *
 * @warn
 * Add/modify/delete note is done by NoteData class.
 * Do not add Note object into NoteData class manually!
 */
class Note : public NoteDesc, public NoteSoundDesc
{
public:
  Note();
  Note(const Note& note);
  Note(Note&& note) = default;
  Note& operator=(const Note& note) = default;
  ~Note();

  NoteType type() const;
  void set_type(NoteType t);
  int player() const;
  void set_player(int player);
  int get_player() const;

  std::vector<NoteDesc*>::iterator begin();
  std::vector<NoteDesc*>::iterator end();
  std::vector<NoteDesc*>::const_iterator begin() const;
  std::vector<NoteDesc*>::const_iterator end() const;

  NoteDesc* NewChain();
  void RemoveAllChain();
  size_t chainsize() const;
  virtual NotePos& endpos();
  using NoteDesc::endpos;

  /**
   * @brief get tapping method for this type of note.
   * 0: generally tapping to all note
   * 1: tapping to all note except to last one - touch-up.
   * 2: first tap, and sliding(key-down).
   */
  int get_tap_type() const;

  /**
   * @brief is key-up allowed while chain is processing.
   */
  bool is_focusout_allowed() const;

  bool operator==(const Note &other) const noexcept;
  std::string toString() const;
  virtual Note* clone() const;

private:
  // type of note
  NoteType type_;

  // note player
  int player_;

  // Chains for this note.
  // @warn first chain is pointing to this Note object.
  std::vector<NoteDesc*> chains_;
};


/**
 * @brief
 * Bgm object
 */
class BgmObject : public NotePos, public NoteSoundDesc
{
public:
  BgmObject();
  void set_bgm_type(int bgm_type);
  void set_column(int col);
  int get_bgm_type() const;
  int get_column() const;
  virtual BgmObject* clone() const;

private:
  int bgm_type_;
  int column_;
};


/**
 * @brief
 * Bga object
 */
class BgaObject : public NotePos
{
public:
  BgaObject();
  void set_layer(uint32_t layer);
  void set_channel(Channel ch);
  uint32_t layer() const;
  Channel channel() const;
  virtual BgaObject* clone() const;

private:
  uint32_t layer_; /* layer id */
  Channel channel_; /* channel to set */
};


/**
 * @brief
 * Timing related object.
 * @warn Position type may be set as Time.
 */
class TimingObject : public NotePos
{
public:
  TimingObject();

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

  virtual TimingObject* clone() const;
  bool operator==(const TimingObject &other) const noexcept;

private:
  void set_type(int type);
  int get_type() const;
  union
  {
    float f;
    int32_t i;
  } value;
  virtual std::string getValueAsString() const;
};


/**
 * @brief
 * Other event related objects (not tempo / sound related; BGA, ...)
 */
class EffectObject : public NotePos
{
public:
  EffectObject();

  void SetMidiCommand(uint8_t command, uint8_t arg1, uint8_t arg2 = 0);
  void SetBmsARGBCommand(BgaTypes bgatype, Channel channel);

  void GetBga(int &bga_type, Channel &channel) const;
  void GetMidiCommand(uint8_t &command, uint8_t &arg1, uint8_t &arg2) const;

  virtual EffectObject* clone() const;
  bool operator==(const EffectObject &other) const noexcept;

private:
  int type_;
  int32_t command_, arg1_, arg2_;

  void set_type(int type);
  int get_type() const;
  virtual std::string getValueAsString() const;
};

/**
 * @brief
 * Contains objects for single track.
 * All objects are always sorted by beat position,
 * and used for adding object without position duplication.
 * (Object position duplication is also available
 *  with AddObjectDuplicated())
 *
 * @warn
 * All object's postype/track should be Beat,
 * and should not modified outside TrackData.
 */
class Track
{
public:
  Track();
  ~Track();

  void AddObject(NotePos* object);
  void AddObjectDuplicated(NotePos* object);
  void RemoveObject(NotePos* object);
  NotePos* GetObjectByPos(int measure, int nu, int de);
  NotePos* GetObjectByMeasure(double measure);
  void RemoveObjectByPos(int measure, int nu, int de);
  void RemoveObjectByMeasure(double measure);
  bool IsHoldNoteAt(double measure) const;
  bool IsRangeEmpty(double m_start, double m_end) const;
  void GetObjectByRange(double m_start, double m_end, std::vector<NotePos*> &out);

  void ClearAll();
  void ClearRange(double m_begin, double m_end);
  void CopyRange(const Track& from, double m_begin, double m_end);
  void CopyAll(const Track& from);
  void MoveRange(double m_delta, double m_begin, double m_end);
  void MoveAll(double m_delta);
  void InsertBlank(double m_begin, double m_delta);

  typedef std::vector<NotePos*>::iterator iterator;
  typedef std::vector<NotePos*>::const_iterator const_iterator;
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  NotePos& front();
  NotePos& back();
  void swap(Track &track);
  size_t size() const;
  bool is_empty() const;
  void clear();

protected:
  std::vector<NotePos*> objects_;
};

/* @brief TrackData class with implicit type casting */
template <typename T>
class TrackWithType : public Track
{
public:
  void AddObject(T* object)
  {
    TrackData::AddObject(object);
  }

  void AddObjectDuplicated(NotePos* object)
  {
    TrackData::AddObjectDuplicated(object);
  }

  void RemoveObject(T* object)
  {
    TrackData::RemoveObject(object);
  }

  T* GetObjectByPos(int measure, int nu, int de)
  {
    TrackData::GetObjectByPos(measure, nu, de);
  }

  T* GetObjectByMeasure(double measure)
  {
    TrackData::GetObjectByBeat(measure);
  }
};

constexpr size_t kMaxTrackSize = 128;

/**
 * @brief Objects with multiple tracks.
 *
 * @warn
 * All object's postype/track should be Beat,
 * and should not modified outside TrackData.
 */
class TrackData
{
public:
  TrackData();

  void set_track_count(int track_count);
  int get_track_count() const;
  void auto_set_track_count();

  void AddObject(NotePos* object);
  void AddObjectDuplicated(NotePos* object);
  void RemoveObject(NotePos* object);
  NotePos* GetObjectByPos(size_t track, int measure, int nu, int de);
  NotePos* GetObjectByMeasure(size_t track, double measure);
  NotePos** GetRowByPos(int measure, int nu, int de);
  NotePos** GetRowByMeasure(double measure);
  void RemoveObjectByPos(int measure, int nu, int de);
  void RemoveObjectByMeasure(double measure);
  void RemoveObjectByPos(size_t track, int measure, int nu, int de);
  void RemoveObjectByMeasure(size_t track, double measure);
  bool IsHoldNoteAt(double measure) const;
  bool IsRangeEmpty(double beat_start, double m_end) const;
  void GetNoteByRange(double beat_start, double beat_end, std::vector<NotePos*> &out);
  void GetAllTrackNote(std::vector<NotePos*> &out);

  void ClearAll();
  void ClearTrack(size_t track);
  void ClearRange(double beat_begin, double beat_end);
  void CopyRange(const TrackData& from, double beat_begin, double beat_end);
  void CopyAll(const TrackData& from);
  void MoveRange(double beat_delta, double beat_begin, double beat_end);
  void MoveAll(double beat_delta);
  void InsertBlank(double beat_begin, double beat_delta);
  void RemapTracks(size_t *track_map);
  void UpdateTracks();

  typedef std::vector<NotePos*>::iterator iterator;
  typedef std::vector<NotePos*>::const_iterator const_iterator;
  iterator begin(size_t track);
  iterator end(size_t track);
  const_iterator begin(size_t track) const;
  const_iterator end(size_t track) const;
  NotePos* front();
  NotePos* back();
  const NotePos* front() const;
  const NotePos* back() const;
  Track& get_track(size_t track);
  const Track& get_track(size_t track) const;

  /* all track iterator */
  class all_track_iterator
  {
  public:
    NotePos* p();
    void next();
    all_track_iterator &operator++();
    NotePos& operator*();
    const NotePos& operator*() const;
    bool is_end() const;
    friend class TrackData;
  private:
    std::vector<NotePos*> notes_;
    std::vector<NotePos*>::iterator iter_;
  };
  all_track_iterator GetAllTrackIterator() const;
  all_track_iterator GetAllTrackIterator(double m_start, double m_end) const;

  /* row number iterator */
  class row_iterator
  {
  public:
    row_iterator();
    row_iterator(double m_start, double m_end);
    double get_measure() const;
    void next();
    bool is_end() const;
    NotePos* col(size_t idx);
    row_iterator &operator++();
    row_iterator operator++(int) { return operator++(); }
    NotePos& operator[](size_t i);
    const NotePos& operator[](size_t i) const;
    double operator*() const;
    friend class TrackData;
  private:
    double m_start, m_end;
    double measure;
    all_track_iterator all_track_iter_;
    NotePos* curr_row_notes[kMaxTrackSize];
  };
  row_iterator GetRowIterator() const;
  row_iterator GetRowIterator(double m_start, double m_end) const;

  void swap(TrackData &data);
  size_t size() const;
  bool is_empty() const;
  void clear();
  std::string toString() const;

private:
  // XXX: max track size is 128?
  Track track_[kMaxTrackSize];

  // track data of currently selected row by GetTracks
  NotePos* track_row_[kMaxTrackSize];

  // number of track
  int track_count_;

  // number of track count for shuffle
  int track_count_shuffle_;
};

/* @brief TrackData with type */
template <typename T>
class TrackDataWithType : public TrackData
{
public:
  void AddObject(T* object)
  {
    TrackData::AddObject(object);
  }

  void AddObjectDuplicated(T* object)
  {
    TrackData::AddObjectDuplicated(object);
  }

  void RemoveObject(T* object)
  {
    TrackData::RemoveObject(object);
  }

  T* GetObjectByPos(size_t track, int measure, int nu, int de)
  {
    return static_cast<T*>(TrackData::GetObjectByPos(track, measure, nu, de));
  }

  T* GetObjectByBeat(size_t track, double beat)
  {
    return static_cast<T*>(TrackData::GetObjectByBeat(track, beat));
  }

  T** GetRowByPos(int measure, int nu, int de)
  {
    return static_cast<T**>(TrackData::GetRowByPos(measure, nu, de));
  }

  T** GetRowByBeat(double beat)
  {
    return static_cast<T**>(TrackData::GetRowByPos(beat));
  }
};

using NoteData = TrackDataWithType<Note>;
using EffectData = TrackDataWithType<EffectObject>;
using BgmData = TrackDataWithType<BgmObject>;
using BgaData = TrackDataWithType<BgaObject>;
//using TimingData = ObjectDataWithType<EffectObject>;

}

#endif
