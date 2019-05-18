#include "Note.h"

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
  type = NotePosTypes::Row;
  if (deno == 0) this->measure = measure;
  else this->measure = measure + (double)num / deno;
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
  case NotePosTypes::Row:
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
  sType = kNoteTypesStr[type];
  sSubtype = pNoteSubtypeStr[type][subtype];
  ss << "[Object Note]\n";
  ss << "type/subtype: " << sType << "," << sSubtype << std::endl;
  ss << NotePos::toString();
  ss << getValueAsString();
  return ss.str();
}

NotePos& NotePos::GetNotePos()
{
  return *this;
}

const NotePos& NotePos::GetNotePos() const
{
  return const_cast<NotePos*>(this)->GetNotePos();
}

NotePosTypes NotePos::GetNotePosType() const
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

NoteType Note::GetNotetype() const
{
  return type;
}

NoteType Note::GetNoteSubtype() const
{
  return subtype;
}

void Note::SetNotetype(NoteType t)
{
  type = t;
}

void Note::SetNoteSubtype(NoteType t)
{
  subtype = t;
}

bool Note::operator==(const Note &other) const noexcept
{
  return NotePos::operator==(other) && type == other.type && subtype == other.subtype;
}

SoundNote::SoundNote() : value(0), volume(1.0f), pitch(0), restart(false)
{
  SetAsBGM(0);
}

void SoundNote::SetAsBGM(uint8_t col)
{
  SetNotetype(NoteTypes::kBGM);
  track.lane.note.player = 0;
  track.lane.note.lane = col;
}

void SoundNote::SetAsTouchNote()
{
  SetNotetype(NoteTypes::kTouch);
}

void SoundNote::SetAsTapNote(uint8_t player, uint8_t lane)
{
  SetNotetype(NoteTypes::kNote);
  track.lane.note.player = player;
  track.lane.note.lane = lane;
}

void SoundNote::SetAsChainNote()
{
  SetNotetype(NoteTypes::kChain);
}

void SoundNote::SetLongnoteLength(double delta_beat)
{
  ASSERT(GetNotetype() == NoteTypes::kNote);
  ASSERT(GetNotePosType() == NotePosTypes::Beat);
  if (!IsLongnote())
    chains.emplace_back(NoteChain{ track, *this, 0 });
  chains.back().pos.SetBeatPos(GetNotePos().beat + delta_beat);
}

void SoundNote::SetLongnoteEndPos(const NotePos& row_pos)
{
  ASSERT(GetNotetype() == NoteTypes::kNote);
  if (!IsLongnote())
    chains.emplace_back(NoteChain{ track, *this, 0 });
  chains.back().pos = row_pos;
}

void SoundNote::SetLongnoteEndValue(Channel v)
{
  ASSERT(GetNotetype() == NoteTypes::kNote);
  if (!IsLongnote())
    chains.emplace_back(NoteChain{ track, *this, 0 });
  chains.back().value = v;
}

void SoundNote::AddChain(const NotePos& pos, uint8_t col)
{
  ASSERT(GetNotetype() == NoteTypes::kChain);
  NoteTrack track;
  track.lane.note.player = 0;
  track.lane.note.lane = col;
  chains.emplace_back(NoteChain{ track, pos, 0 });
}

void SoundNote::AddTouch(const NotePos& pos, uint8_t x, uint8_t y)
{
  ASSERT(GetNotetype() == NoteTypes::kTouch);
  NoteTrack track;
  track.lane.touch.x = x;
  track.lane.touch.y = y;
  chains.emplace_back(NoteChain{ track, pos, 0 });
}

void SoundNote::ClearChain()
{
  chains.clear();
}

bool SoundNote::IsLongnote()
{
  return chains.size() > 0;
}

uint8_t SoundNote::GetPlayer()
{
  ASSERT(GetNotetype() == NoteTypes::kNote);
  return track.lane.note.player;
}

uint8_t SoundNote::GetLane()
{
  ASSERT(GetNotetype() == NoteTypes::kNote);
  return track.lane.note.lane;
}

uint8_t SoundNote::GetBGMCol()
{
  ASSERT(GetNotetype() == NoteTypes::kBGM);
  return track.lane.note.lane;
}

uint8_t SoundNote::GetX()
{
  ASSERT(GetNotetype() == NoteTypes::kTouch);
  return track.lane.touch.x;
}

uint8_t SoundNote::GetY()
{
  ASSERT(GetNotetype() == NoteTypes::kTouch);
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

std::string BgaNote::getValueAsString() const
{
  std::stringstream ss;
  ss << "Value (channel): " << value << std::endl;
  return ss.str();
}

BgaNote::BgaNote() : value(0)
{
  SetNotetype(NoteTypes::kBGA);
}

bool BgaNote::operator==(const BgaNote &other) const noexcept
{
  return Note::operator==(other) && value == other.value;
}

TempoNote::TempoNote()
{
  SetNotetype(NoteTypes::kTempo);
}

void TempoNote::SetBpm(float bpm)
{
  SetNoteSubtype(NoteTempoTypes::kBpm);
  value.f = bpm;
}

void TempoNote::SetBmsBpm(Channel bms_channel)
{
  SetNoteSubtype(NoteTempoTypes::kBmsBpm);
  value.i = bms_channel;
}

void TempoNote::SetStop(float stop)
{
  SetNoteSubtype(NoteTempoTypes::kStop);
  value.f = stop;
}

void TempoNote::SetBmsStop(Channel bms_channel)
{
  SetNoteSubtype(NoteTempoTypes::kBmsStop);
  value.i = bms_channel;
}

void TempoNote::SetMeasure(float measure_length)
{
  SetNoteSubtype(NoteTempoTypes::kMeasure);
  value.f = measure_length;
}

void TempoNote::SetScroll(float scrollspeed)
{
  SetNoteSubtype(NoteTempoTypes::kScroll);
  value.f = scrollspeed;
}

void TempoNote::SetTick(int32_t tick)
{
  SetNoteSubtype(NoteTempoTypes::kTick);
  value.i = tick;
}

void TempoNote::SetWarp(float warp_to)
{
  SetNoteSubtype(NoteTempoTypes::kWarp);
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