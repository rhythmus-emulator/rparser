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

Row::Row(uint32_t measure, RowPos num, RowPos deno)
  : measure(measure), num(num), deno(deno) {}

std::string Note::toString() const
{
  std::stringstream ss;
  std::string sType, sSubtype;
  sType = kNoteTypesStr[type];
  sSubtype = pNoteSubtypeStr[type][subtype];
  ss << "[Object Note]\n";
  ss << "type/subtype: " << sType << "," << sSubtype;
  ss << "Beat: " << pos.beat << std::endl;
  ss << "Row: " << pos.row.measure << " " << pos.row.num << "/" << pos.row.deno << std::endl;
  ss << "Time: " << pos.time_msec << std::endl;
  ss << getValueAsString() << std::endl;
  return ss.str();
}

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

void Note::SetRowPos(const Row& r)
{
  pos.type = NotePosTypes::Row;
  pos.row = r;
}

void Note::SetRowPos(uint32_t measure, RowPos deno, RowPos num)
{
  pos.type = NotePosTypes::Row;
  pos.row = std::move(Row(measure,deno,num));
}

void Note::SetBeatPos(double beat)
{
  pos.type = NotePosTypes::Beat;
  pos.beat = beat;
}

void Note::SetTimePos(double time_msec)
{
  pos.type = NotePosTypes::Time;
  pos.time_msec = time_msec;
}

NotePos& Note::GetNotePos()
{
  return pos;
}

const NotePos& Note::GetNotePos() const
{
  return const_cast<Note*>(this)->GetNotePos();
}

NotePosTypes Note::GetNotePosType() const
{
  return pos.type;
}

double Note::GetBeatPos() const
{
  return pos.beat;
}

double Note::GetTimePos() const
{
  return pos.time_msec;
}

bool NotePos::operator==(const NotePos &other) const noexcept
{
  if (type != other.type) return false;
  switch (type)
  {
  case NotePosTypes::Beat:
    return beat == other.beat;
  case NotePosTypes::Row:
    return row.measure == other.row.measure &&
      row.num / (double)row.deno == other.row.num / (double)other.row.deno;
  case NotePosTypes::Time:
    return time_msec == other.time_msec;
  default:
    ASSERT(0);
    return false;
  }
}

bool Note::operator<(const Note &other) const noexcept
{
  return pos.beat < other.pos.beat;
}

bool Note::operator==(const Note &other) const noexcept
{
  return pos == other.pos;
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

void SoundNote::SetAsKnobNote()
{
  SetNotetype(NoteTypes::kKnob);
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