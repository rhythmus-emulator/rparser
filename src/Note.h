#ifndef RPARSER_NOTE_H
#define RPARSER_NOTE_H

#include <string>
#include <vector>

namespace rparser
{

typedef uint16_t RowPos;
typedef uint8_t NoteType;
typedef uint32_t Channel;

/* @brief Description about note chain status */
enum class NoteChainStatus
{
  Tap,
  Start,
  Body,
  End
};

/* @brief Sound property of note */
struct NoteSoundProperty
{
  int type;
  int length;
  int key;
  float volume;
};

/**
 * @brief General object contained in track with time/beat information.
 * @warn  Value storing is not compatible between int and double. Use only one type of value.
 * @warn  'eq' operator only compares Note beat position, not value.
 *
 * Note object should be compatible to any type of track,
 * but detailed property is not cleared when it moves between tracks.
 * e.g. Note in tapping track can move to Auto track without any error,
 * but detailed information (such as value) might be lost / corrupted.
 */
class NoteElement
{
public:
  NoteElement();

  void set_measure(double measure);
  void set_time(double time_msec);
  void set_chain_status(NoteChainStatus cstat);
  double measure() const;
  double time() const;
  NoteChainStatus chain_status() const;
  void SetRowPos(int measure, RowPos deno, RowPos num);
  void SetDenominator(uint32_t denominator);
  bool operator==(const NoteElement &other) const noexcept;
  bool operator<(const NoteElement &other) const noexcept;
  std::string toString() const;

  void set_point(int x);
  void set_point(int x, int y);
  void set_point(int x, int y, int z);
  void get_point(int &x);
  void get_point(int &x, int &y);
  void get_point(int &x, int &y, int &z);

  void set_value(int v);
  void set_value(unsigned v);
  void set_value(double v);
  void set_value(NoteSoundProperty &sprop);
  void get_value(int &v);
  int get_value_i() const;
  unsigned int get_value_u() const;
  double get_value_f() const;
  const NoteSoundProperty& get_value_sprop() const;
  NoteSoundProperty& get_value_sprop();

private:
  // Note time/beat position.
  double measure_;
  double time_msec_;
  RowPos num_, deno_;
  NoteChainStatus chain_status_;

  // Note location. used for tapping note, or BGM Note column.
  // Use second array to consider note as rectangular one.
  struct NotePoint {
    int x, y, z;
  } point_;

  // Property of the note.
  union NoteValue {
    int i[4];
    unsigned u[4];
    int64_t li;
    uint64_t lu;
    double f;
    NoteSoundProperty sprop;
  } v_;
};

/* @brief Types of tracks
 * TrackBGM : Only stores BGM notes but with track number information (used for BMS) */
enum TrackTypes
{
  kTrackTiming,
  kTrackTap,
  kTrackCommand,
  kTrackBGM,
  kTrackMax
};

/* @brief Special object which changes tempo of the chart. */
enum TimingTrackTypes
{
  kMeasure,         // Bms type bar length
  kScroll,          // Stepmania type bar length
  kBpm,
  kStop,
  kWarp,
  kTick,
  kBmsBpm,          // #BPM / #EXBPM command from metadata 
  kBmsStop,         // #STOP command from metadata
  kTimingTrackMax
};

/* @brief Automonous played track for notes which affect to game (expect timing) */
enum CommandTrackTypes
{
  kBgaMiss,
  kBgaMain,
  kBgaLayer1,
  kBgaLayer2,
  kBgm,
  kMidi,
  kBmsKeyBind,      // key bind layer (bms #SWBGA)
  kBmsEXTCHR,       // #EXTCHR cmd from BMS. (not supported)
  kBmsTEXT,         // #TEXT / #SONG command in BMS
  kBmsBMSOPTION,    // #CHANGEOPTION command in BMS
  kBmsARGBLAYER,    // BGA opacity setting (#ARGB channel)
};

/** @detail Detailed note types */
enum NoteTypes
{
  kNormalNote,      // general tappable note (includes longnote)
  kInvisibleNote,   // invisible and no damage, but scorable. changes keysound (bms)
  kMineNote,        // shock / mine note.
  kAutoNote,        // drawn and sound but not judged,
  kFakeNote,        // drawn but not judged nor sound.
  kFreeNote,        // free combo area (Taigo yellow note / DJMAX stick rotating)
  kDragNote,        // dragging longnote, check key_is_pressed input only. (TECHNICA / deemo / SDVX VEFX ...)
  kRepeatNote,      // repeat longnote (TECHNICA)
};


class Track;

/**
 * @brief An object consisted with NoteElement. Consisted with multiple NoteElements if Longnote object.
 *        Mainly used for note iterating.
 * @warn  May behave wrong if track data is modified during usage of Note object.
 */
class Note
{
public:
  Note(Track* track);
  size_t size() const;
  bool next();
  bool prev();
  void reset();
  void SeekByBeat(double beat);
  void SeekByTime(double time);
  bool is_end() const;
  NoteElement* get();
  const NoteElement* get() const;
  friend class Track;
private:
  Track* track_;
  size_t index_, size_;
};


/**
 * @brief Contains objects for single track.
 *
 * All objects are always sorted by beat position,
 * and used for adding object without position duplication.
 * (Object position duplication is also available
 *  with AddObjectDuplicated())
 *
 * track_datatype_ is used to check whether NoteElement data is compatible
 * when NoteElement is moved/copied between Tracks.
 * If not compatible, then value of the NoteElement will be wipe out,
 * only measure value would be left.
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

  void AddNoteElement(const NoteElement& object);
  void RemoveNoteElement(const NoteElement& object);
  NoteElement* GetNoteElementByPos(int measure, int nu, int de);
  NoteElement* GetNoteElementByMeasure(double measure);
  NoteElement* get(size_t index);
  void RemoveNoteByPos(int measure, int nu, int de);
  void RemoveNoteByMeasure(double measure);
  void ClearAll();
  void ClearRange(double m_begin, double m_end);
  void SetObjectDupliable(bool duplicable);

  bool IsRangeEmpty(double measure) const;
  bool IsRangeEmpty(double m_start, double m_end) const;
  bool IsHoldNoteAt(double measure) const;
  bool HasLongnote() const;

  // If ranged note is spanned, then all NoteElement are returned.
  void GetNoteElementsByRange(double m_start, double m_end, std::vector<NoteElement*> &out);
  void GetAllNoteElements(std::vector<NoteElement*> &out);

  void CopyRange(const Track& from, double m_begin, double m_end);
  void CopyAll(const Track& from);
  void MoveRange(double m_delta, double m_begin, double m_end);
  void MoveAll(double m_delta);
  void InsertBlank(double m_begin, double m_delta);

  typedef std::vector<NoteElement>::iterator iterator;
  typedef std::vector<NoteElement>::const_iterator const_iterator;
  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  NoteElement& front();
  NoteElement& back();
  void swap(Track &track);
  size_t size() const;
  bool is_empty() const;
  void clear();

protected:
  std::vector<NoteElement> notes_;
  std::string track_datatype_;
  bool is_object_duplicable_;
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

  void set_track_count(size_t track_count);
  size_t get_track_count() const;
  Track& get_track(size_t track);
  const Track& get_track(size_t track) const;
  Track& operator[](size_t track);
  const Track& operator[](size_t track) const;

  NoteElement* GetObjectByPos(int measure, int nu, int de);
  NoteElement* GetObjectByMeasure(double measure);
  void RemoveObjectByPos(int measure, int nu, int de);
  void RemoveObjectByMeasure(double measure);
  void ClearAll();
  void ClearRange(double m_begin, double m_end);
  void SetObjectDupliable(bool duplicable);
  bool IsHoldNoteAt(double measure) const;
  bool IsRangeEmpty(double m_start, double m_end) const;
  bool HasLongnote() const;

  void CopyRange(const TrackData& from, double m_begin, double m_end);
  void CopyAll(const TrackData& from);
  void MoveRange(double m_delta, double m_begin, double m_end);
  void MoveAll(double m_delta);
  void InsertBlank(double m_begin, double m_delta);
  void RemapTracks(size_t *track_map);

  unsigned GetNoteElementCount() const;
  unsigned GetNoteCount() const;
  NoteElement* front();
  NoteElement* back();
  const NoteElement* front() const;
  const NoteElement* back() const;
  void clear();

  /* all notes iterator */
  class all_track_iterator
  {
  public:
    all_track_iterator(TrackData *td);
    all_track_iterator(TrackData *td, double m_start, double m_end);
    all_track_iterator &operator++();
    all_track_iterator operator++(int) { return operator++(); }
    double get_measure() const;
    size_t get_track() const;
    NoteElement* get();
    NoteElement& operator*();
    const NoteElement& operator*() const;
    void next();
    bool is_end() const;
  private:
    std::vector<NoteElement*> all_notes_;
    std::vector<size_t> track_numbers_;
    size_t idx_;
  };
  all_track_iterator GetAllTrackIterator();
  all_track_iterator GetAllTrackIterator(double m_start, double m_end);

  /* row number iterator */
  class row_iterator : public all_track_iterator
  {
  public:
    row_iterator(TrackData *td); // TODO: set start measure properly (not always zero; call next() by default.)
    row_iterator(TrackData *td, double m_start, double m_end);
    double get_measure() const;
    void next();
    bool is_end() const;
    bool is_longnote() const;
    row_iterator &operator++();
    row_iterator operator++(int) { return operator++(); }
    NoteElement* get(size_t column);
    NoteElement& operator[](size_t column);
    const NoteElement& operator[](size_t i) const;
    double operator*() const;
    friend class TrackData;
  private:
    bool is_row_iter_end;
    size_t track_count;
    double measure;
    NoteElement* curr_row_note_elem[kMaxTrackSize];
    bool curr_row_longnote[kMaxTrackSize];
  };
  row_iterator GetRowIterator();
  row_iterator GetRowIterator(double m_start, double m_end);

  void swap(TrackData &data);
  size_t size() const;
  bool is_empty() const;
  std::string toString() const;

private:
  std::string name_;
  std::vector<Track> tracks_;
  std::string track_datatype_default_;
  bool is_object_duplicable_;
};

}

#endif
