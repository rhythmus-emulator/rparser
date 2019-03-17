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

std::vector<Note>::iterator NoteData::begin() { return notes_.begin(); }
std::vector<Note>::iterator NoteData::end() { return notes_.end(); }
const std::vector<Note>::const_iterator NoteData::begin() const { return notes_.begin(); }
const std::vector<Note>::const_iterator NoteData::end() const { return notes_.end(); }

std::string Note::toString() const
{
  std::stringstream ss;
  std::string sType, sSubtype;
  sType = kNoteTypesStr[track.type];
  sSubtype = pNoteSubtypeStr[track.type][track.subtype];
  ss << "[Object Note]\n";
  ss << "type/subtype: " << sType << "," << sSubtype;
  ss << "Value (int): " << value.i << std::endl;
  ss << "Value (float): " << value.f << std::endl;
  ss << "Beat: " << pos.beat << std::endl;
  ss << "Row: " << pos.row.measure << " " << pos.row.num << "/" << pos.row.deno << std::endl;
  ss << "Time: " << pos.time_msec << std::endl;
  return ss.str();
}

bool NotePos::operator==(const NotePos &other) const
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
  }
}

bool Note::operator<(const Note &other) const
{
  return pos.beat < other.pos.beat;
}

bool Note::operator==(const Note &other) const
{
  return
    pos == other.pos &&
    value.i == other.value.i;
}

}