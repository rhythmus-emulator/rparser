#include "Note.h"
#include <math.h>

namespace rparser
{

const char *kNoteTypesStr[] = {
  "NONE",
  "NOTE",
  "TOUCH",
  "KNOB",
  "BGM",
  "BGA",
  "TEMPO",
  "SPECIAL",
};

const char *kNoteSubTypesStr[] = {
  "NORMAL",
  "INVISIBLE",
  "LONGNOTE",
  "CHARGE",
  "HCHARGE",
  "MINE",
  "AUTO",
  "FAKE",
  "COMBO",
  "DRAG",
  "CHAIN",
  "REPEAT",
};

const char *kNoteTempoTypesStr[] = {
  "BPM",
  "STOP",
  "WARP",
  "SCROLL",
  "MEASURE",
  "TICK",
  "BMSBPM",
  "BMSSTOP",
};

const char *kNoteBgaTypesStr[] = {
  "MAINBGA",
  "MISSBGA",
  "LAYER1",
  "LAYER2",
  "LAYER3",
  "LAYER4",
  "LAYER5",
  "LAYER6",
  "LAYER7",
  "LAYER8",
  "LAYER9",
  "LAYER10",
};

const char *kNoteSpecialTypesStr[] = {
  "REST",
  "BMSKEYBIND",
  "BMSEXTCHR",
  "BMSTEXT",
  "BMSOPTION",
  "BMSARGBBASE",
  "BMSARGBMISS",
  "BGAARGBLAYER1",
  "BGAARGBLAYER2",
};

const char **pNoteSubtypeStr[] = {
  kNoteSubTypesStr,
  kNoteTempoTypesStr,
  kNoteBgaTypesStr,
  kNoteSpecialTypesStr,
};

NotePos::NotePos()
  : type(NotePosTypes::NullPos), time_msec(0), beat(0), measure(0), denominator(8) {}

void NotePos::SetRowPos(uint32_t measure, RowPos deno, RowPos num)
{
  if (deno == 0) SetRowPos(measure);
  else SetRowPos(measure + (double)num / deno);
}

void NotePos::SetRowPos(double barpos)
{
  type = NotePosTypes::Bar;
  this->measure = barpos;
}

void NotePos::SetBeatPos(double beat)
{
  type = NotePosTypes::Beat;
  this->beat = beat;
}

void NotePos::SetTimePos(double time_msec)
{
  type = NotePosTypes::Time;
  this->time_msec = time_msec;
}

void NotePos::SetDenominator(uint32_t denominator)
{
  this->denominator = denominator;
}

std::string NotePos::toString() const
{
  std::stringstream ss;
  int32_t num = 0;
  switch (type)
  {
  case NotePosTypes::NullPos:
    ss << "(No note pos set)" << std::endl;
    break;
  case NotePosTypes::Beat:
    ss << "Beat: " << beat << "(Deno " << denominator << ")" << std::endl;
    break;
  case NotePosTypes::Time:
    ss << "Time: " << time_msec << "(Deno " << denominator << ")" << std::endl;
    break;
  case NotePosTypes::Bar:
    // recalculate numerator of Row
    num = static_cast<int32_t>(lround(measure * denominator) % denominator);
    ss << "Row: " << (static_cast<int32_t>(measure) % 1) << " " << num << "/" << denominator << std::endl;
    break;
  }
  return ss.str();
}

std::string Note::toString() const
{
  std::stringstream ss;
  std::string sType, sSubtype;
  sType = kNoteTypesStr[type_];
  sSubtype = pNoteSubtypeStr[type_][subtype_];
  ss << "[Object Note]\n";
  ss << "type/subtype: " << sType << "," << sSubtype << std::endl;
  ss << NotePos::toString();
  ss << getValueAsString();
  return ss.str();
}

NotePos& NotePos::pos()
{
  return *this;
}

const NotePos& NotePos::pos() const
{
  return const_cast<NotePos*>(this)->pos();
}

NotePosTypes NotePos::postype() const
{
  return type;
}

double NotePos::GetBeatPos() const
{
  return beat;
}

double NotePos::GetTimePos() const
{
  return time_msec;
}

bool NotePos::operator<(const NotePos &other) const noexcept
{
  return beat < other.beat;
}

bool NotePos::operator==(const NotePos &other) const noexcept
{
  return beat == other.beat;
}

#if 0
bool NotePos::operator==(const NotePos &other) const noexcept
{
  if (type != other.type) return false;
  switch (type)
  {
  case NotePosTypes::Beat:
    return beat == other.beat;
  case NotePosTypes::Time:
    return time_msec == other.time_msec;
  default:
    ASSERT(0);
    return false;
  }
}
#endif

NoteType Note::type() const
{
  return type_;
}

NoteType Note::subtype() const
{
  return subtype_;
}

void Note::set_type(NoteType t)
{
  type_ = t;
}

void Note::set_subtype(NoteType t)
{
  subtype_ = t;
}

bool Note::operator==(const Note &other) const noexcept
{
  return NotePos::operator==(other) && type() == other.type() && subtype() == other.subtype();
}

SoundNote::SoundNote() : value(0), volume(1.0f), pitch(0), restart(false)
{
  SetAsBGM(0);
}

void SoundNote::SetAsBGM(uint8_t col)
{
  set_type(NoteTypes::kBGM);
  track.lane.note.player = 0;
  track.lane.note.lane = col;
}

void SoundNote::SetAsTouchNote()
{
  set_type(NoteTypes::kTouch);
}

void SoundNote::SetAsTapNote(uint8_t player, uint8_t lane)
{
  set_type(NoteTypes::kTap);
  set_subtype(NoteSubTypes::kNormalNote);
  track.lane.note.player = player;
  track.lane.note.lane = lane;
}

void SoundNote::SetAsChainNote()
{
  set_subtype(NoteSubTypes::kLongNote);
}

void SoundNote::SetLongnoteLength(double delta_value)
{
  ASSERT(type() == NoteTypes::kTap || type() == NoteTypes::kTouch);
  if (!IsLongnote())
    chains.emplace_back(NoteChain{ track, *this, 0 });
  switch (postype())
  {
  case NotePosTypes::Beat:
    chains.back().pos.SetBeatPos(pos().beat + delta_value);
    break;
  case NotePosTypes::Bar:
    chains.back().pos.SetRowPos(pos().measure + delta_value);
    break;
  case NotePosTypes::Time:
    chains.back().pos.SetTimePos(pos().time_msec + delta_value);
    break;
  }
}

void SoundNote::SetLongnoteEndPos(const NotePos& row_pos)
{
  ASSERT(type() == NoteTypes::kTap || type() == NoteTypes::kTouch);
  if (!IsLongnote())
    chains.emplace_back(NoteChain{ track, *this, 0 });
  chains.back().pos = row_pos;
}

void SoundNote::SetLongnoteEndValue(Channel v)
{
  ASSERT(type() == NoteTypes::kTap || type() == NoteTypes::kTouch);
  if (!IsLongnote())
    chains.emplace_back(NoteChain{ track, *this, 0 });
  chains.back().value = v;
}

void SoundNote::AddChain(const NotePos& pos, uint8_t col)
{
  ASSERT(type() == NoteTypes::kTap || type() == NoteTypes::kTouch);
  NoteTrack track;
  track.lane.note.player = 0;
  track.lane.note.lane = col;
  chains.emplace_back(NoteChain{ track, pos, 0 });
}

void SoundNote::AddTouch(const NotePos& pos, uint8_t x, uint8_t y)
{
  ASSERT(type() == NoteTypes::kTouch);
  NoteTrack track;
  track.lane.touch.x = x;
  track.lane.touch.y = y;
  chains.emplace_back(NoteChain{ track, pos, 0 });
}

void SoundNote::ClearChain()
{
  chains.clear();
}

bool SoundNote::IsLongnote() const
{
  return chains.size() > 0;
}

bool SoundNote::IsScoreable() const
{
  return type() == NoteTypes::kTap || type() == NoteTypes::kTouch;
}

uint8_t SoundNote::GetPlayer()
{
  ASSERT(type() == NoteTypes::kTap);
  return track.lane.note.player;
}

uint8_t SoundNote::GetLane()
{
  ASSERT(type() == NoteTypes::kTap);
  return track.lane.note.lane;
}

uint8_t SoundNote::GetBGMCol()
{
  ASSERT(type() == NoteTypes::kBGM);
  return track.lane.note.lane;
}

uint8_t SoundNote::GetX()
{
  ASSERT(type() == NoteTypes::kTouch);
  return track.lane.touch.x;
}

uint8_t SoundNote::GetY()
{
  ASSERT(type() == NoteTypes::kTouch);
  return track.lane.touch.y;
}


std::string SoundNote::getValueAsString() const
{
  std::stringstream ss;
  ss << "Track (note - player, lane): " << track.lane.note.player << "," << track.lane.note.lane << std::endl;
  ss << "Track (touch - x, y): " << track.lane.touch.x << "," << track.lane.touch.y << std::endl;
  ss << "Volume: " << volume << std::endl;
  ss << "Pitch: " << pitch << std::endl;
  ss << "Restart (sound) ?: " << restart << std::endl;
  return ss.str();
}

bool SoundNote::operator==(const SoundNote &other) const noexcept
{
  return Note::operator==(other) && value == other.value;
}

std::string EventNote::getValueAsString() const
{
  std::stringstream ss;
  ss << "Command: " << command_;
  if (arg1_) ss << ", arg1: " << arg1_;
  if (arg2_) ss << ", arg2: " << arg2_;
  ss << std::endl;
  return ss.str();
}

EventNote::EventNote() : command_(0), arg1_(0), arg2_(0)
{
  set_type(NoteTypes::kEvent);
}

void EventNote::SetBga(BgaTypes bgatype, Channel channel, uint8_t column)
{
  set_type(NoteEventTypes::kBGA);
  command_ = bgatype;
  arg1_ = static_cast<decltype(arg1_)>(channel);
  arg2_ = column;
}

void EventNote::SetMidiCommand(uint8_t command, uint8_t arg1, uint8_t arg2)
{
  set_type(NoteEventTypes::kMIDI);
  command_ = command;
  arg1_ = arg1;
  arg2_ = arg2;
}

void EventNote::SetBmsARGBCommand(BgaTypes bgatype, Channel channel)
{
  set_type(NoteEventTypes::kBmsARGBLAYER);
  command_ = bgatype;
  arg1_ = static_cast<decltype(arg1_)>(channel);
  arg2_ = 0;
}

bool EventNote::operator==(const EventNote &other) const noexcept
{
  return Note::operator==(other) &&
         command_ == other.command_ &&
         arg1_ == other.arg1_ &&
         arg2_ == other.arg2_;
}

TempoNote::TempoNote()
{
  set_type(NoteTypes::kTempo);
}

void TempoNote::SetBpm(float bpm)
{
  set_subtype(NoteTempoTypes::kBpm);
  value.f = bpm;
}

void TempoNote::SetBmsBpm(Channel bms_channel)
{
  set_subtype(NoteTempoTypes::kBmsBpm);
  value.i = bms_channel;
}

void TempoNote::SetStop(float stop)
{
  set_subtype(NoteTempoTypes::kStop);
  value.f = stop;
}

void TempoNote::SetBmsStop(Channel bms_channel)
{
  set_subtype(NoteTempoTypes::kBmsStop);
  value.i = bms_channel;
}

void TempoNote::SetMeasure(float measure_length)
{
  set_subtype(NoteTempoTypes::kMeasure);
  value.f = measure_length;
}

void TempoNote::SetScroll(float scrollspeed)
{
  set_subtype(NoteTempoTypes::kScroll);
  value.f = scrollspeed;
}

void TempoNote::SetTick(int32_t tick)
{
  set_subtype(NoteTempoTypes::kTick);
  value.i = tick;
}

void TempoNote::SetWarp(float warp_to)
{
  set_subtype(NoteTempoTypes::kWarp);
  value.f = warp_to;
}

float TempoNote::GetFloatValue() const
{
  return value.f;
}

int32_t TempoNote::GetIntValue() const
{
  return value.i;
}

std::string TempoNote::getValueAsString() const
{
  std::stringstream ss;
  ss << "Value (float): " << value.f << std::endl;
  ss << "Value (int): " << value.i << std::endl;
  return ss.str();
}

bool TempoNote::operator==(const TempoNote &other) const noexcept
{
  return Note::operator==(other) &&
         value.i == other.value.i &&
         value.f == other.value.f;
}

}
