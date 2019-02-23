#include "Chart.h"
#include "ChartUtil.h"
#include "rutil.h"

using namespace rutil;

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

Chart::Chart(const MetaData *md, const NoteData *nd)
  : metadata_shared_(md), notedata_shared_(nd)
{
}

Chart::Chart(const Chart &nd)
{
  Chart(&nd.GetSharedMetaData(), &nd.GetSharedNoteData());
  for (const auto& note : nd.notedata_)
  {
    notedata_.AddNote(Note(note));
  }
  for (const auto& stmt : nd.stmtdata_)
  {
    stmtdata_.push_back(ConditionStatement(stmt));
  }
}

Chart::~Chart()
{
}

void Chart::swap(Chart& c)
{
  notedata_.swap(c.notedata_);
  stmtdata_.swap(c.stmtdata_);
  tempodata_.swap(c.tempodata_);
  metadata_.swap(c.metadata_);
}


void Chart::Clear()
{
  notedata_.clear();
  stmtdata_.clear();
  tempodata_.clear();
  metadata_.clear();
}

void Chart::AppendStmt(ConditionStatement& stmt)
{
  stmtdata_.push_back(stmt);
}

void Chart::EvaluateStmt(int seed)
{
  for (const ConditionStatement& stmt : stmtdata_)
  {
    Chart *c = stmt.EvaluateSentence(seed);
    if (c)
      notedata_.Merge(c->GetNoteData());
  }
}

NoteData& Chart::GetNoteData()
{
  return notedata_;
}

MetaData& Chart::GetMetaData()
{
  return metadata_;
}

const NoteData& Chart::GetNoteData() const
{
  return notedata_;
}

const TempoData& Chart::GetTempoData() const
{
  return tempodata_;
}

const MetaData& Chart::GetMetaData() const
{
  return metadata_;
}

const NoteData& Chart::GetSharedNoteData() const
{
  return notedata_;
}

const MetaData& Chart::GetSharedMetaData() const
{
  return metadata_;
}

void Chart::InvalidateAllNotePos()
{
  std::vector<double> v_time, v_beat;
  std::vector<NotePos::Row> v_row;
  std::vector<double> v_row_to_beat;
  int t_idx = 0, b_idx = 0, r_idx = 0;
  // Sort by row / beat / time.
  SortedNoteObjects sorted;
  SortNoteObjectsByType(GetNoteData(), sorted);
  // Make conversion of row --> beat --> time first.
  for (const Note* nobj : sorted.nobj_by_row)
    v_row.push_back(nobj->pos.row);
  const std::vector<double>&& v_row_to_beat = tempodata_.GetBeatFromRowArr(v_row);
  for (Note* nobj : sorted.nobj_by_row)
    v_beat.push_back(nobj->pos.beat = v_row_to_beat[r_idx++]);
  const std::vector<double>&& v_row_to_time = tempodata_.GetTimeFromBeatArr(v_beat);
  r_idx = 0;
  for (Note* nobj : sorted.nobj_by_row)
    nobj->pos.time_msec = v_row_to_time[r_idx++];
  v_beat.clear();
  // Make conversion of beat <--> time.
  for (const Note* nobj : sorted.nobj_by_beat)
    v_beat.push_back(nobj->pos.beat);
  for (const Note* nobj : sorted.nobj_by_tempo)
    v_time.push_back(nobj->pos.time_msec);
  const std::vector<double>&& v_time_to_beat = tempodata_.GetBeatFromTimeArr(v_time);
  const std::vector<double>&& v_beat_to_time = tempodata_.GetTimeFromBeatArr(v_beat);
  for (Note* nobj : sorted.nobj_by_beat)
    nobj->pos.time_msec = v_beat_to_time[t_idx++];
  for (Note* nobj : sorted.nobj_by_tempo)
    nobj->pos.beat = v_time_to_beat[b_idx++];
}

void Chart::InvalidateNotePos(Note &nobj)
{
  switch (nobj.pos.type)
  {
    case NotePosTypes::Time:
      nobj.pos.beat = tempodata_.GetBeatFromTime(nobj.pos.time_msec);
      break;
    case NotePosTypes::Row:
      nobj.pos.beat = tempodata_.GetBeatFromRow(nobj.pos.row);
    case NotePosTypes::Beat:
      nobj.pos.time_msec = tempodata_.GetTimeFromBeat(nobj.pos.beat);
      break;
  }
}

void Chart::InvalidateTempoData()
{
  TempoData new_tempodata(*this);
  tempodata_.swap(new_tempodata);
}

void ConditionStatement::AddSentence(unsigned int cond, Chart* chartdata)
{
  sentences_[cond] = chartdata;
}

Chart* ConditionStatement::EvaluateSentence(int seed) const
{
  unsigned int cur_seed = seed;
  if (cur_seed < 0)
  {
    cur_seed = rand() % value_;
  }
  auto it = sentences_.find(cur_seed);
  if (it == sentences_.end())
    return 0;
  return it->second;
}

NoteData::NoteData() : is_sorted_(false)
{

}

NoteData::NoteData(const NoteData& notedata)
  : notes_(notedata.notes_), is_sorted_(notedata.is_sorted_)
{

}

void NoteData::swap(NoteData& notedata)
{
  std::swap(notes_, notedata.notes_);
  std::swap(is_sorted_, notedata.is_sorted_);
}

void NoteData::clear()
{
  notes_.clear();
  is_sorted_ = false;
}

void NoteData::Merge(const NoteData &notedata, RowPos rowFrom)
{
  // only merge note data
  // TODO: implement rowFrom
  notes_.insert(notes_.end(), notedata.begin(), notedata.end());
}

void NoteData::SortByBeat()
{
  if (is_sorted_) return;
  std::sort(begin(), end());
  is_sorted_ = true;
}

void NoteData::SortModeOff()
{
  is_sorted_ = false;
}

bool NoteData::IsSorted()
{
  return is_sorted_;
}

void NoteData::AddNote(const Note &n)
{
  if (is_sorted_)
  {
    auto it = std::upper_bound(begin(), end(), n);
    notes_.insert(it, n);
  } else notes_.push_back(n);
}

void NoteData::AddNote(Note &&n)
{
  if (is_sorted_)
  {
    auto it = std::upper_bound(begin(), end(), n);
    notes_.insert(it, n);
  } else notes_.push_back(n);
}

std::vector<Note>::iterator NoteData::begin() { return notes_.begin(); }
std::vector<Note>::iterator NoteData::end()  { return notes_.end(); }
const std::vector<Note>::const_iterator NoteData::begin() const  { return notes_.begin(); }
const std::vector<Note>::const_iterator NoteData::end() const  { return notes_.end(); }

ConditionStatement::ConditionStatement(int value, bool israndom, bool isswitchstmt)
  : value_(value), israndom_(israndom), isswitchstmt_(isswitchstmt)
{
  assert(value_ > 0);
}

ConditionStatement::~ConditionStatement()
{
  for (auto& p : sentences_)
  {
    delete p.second;
  }
}

} /* namespace rparser */
