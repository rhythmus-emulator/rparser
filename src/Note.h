#ifndef RPARSER_NOTE_H
#define RPARSER_NOTE_H

#include "common.h"

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
      struct { uint8_t player, col; } note;
      struct { uint8_t x, y; } touch;
    } lane;
  } NoteTrack;

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
  } row;

  struct NotePos
  {
    NotePosTypes type;
    Row row;
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
    void Cut(double start_beat, double end_beat);
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
}

#endif