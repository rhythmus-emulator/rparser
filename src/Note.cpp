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
  return pos == other.pos && value == other.value;
}

std::string BgaNote::getValueAsString() const
{
  std::stringstream ss;
  ss << "Value (channel): " << value << std::endl;
  return ss.str();
}

bool BgaNote::operator==(const BgaNote &other) const noexcept
{
  return pos == other.pos && value == other.value;
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
  return pos == other.pos &&
         value.i == other.value.i &&
         value.f == other.value.f;
}

}