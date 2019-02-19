#include "Chart.h"
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
    notedata_.push_back(Note(note));
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
  tempodata_.Clear();
  metadata_.Clear();
}

void Chart::MergeNotedata(const Chart &cd, RowPos rowFrom)
{
  // only merge note data
  notedata_.insert(notedata_.end(), cd.notedata_.begin(), cd.notedata_.end());
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
      MergeNotedata(*c);
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
  std::vector<double> v_time_to_beat, v_beat_to_time;
  int t_idx = 0, b_idx = 0;
  for (const Note& nobj : notedata_)
  {
    if (nobj.pos.type == NotePosTypes::Time)
    {
      v_time.push_back(nobj.pos.time_msec);
    } else /* Row type is already converted to beat TODO: set(convert) row too... */
    {
      v_beat.push_back(nobj.pos.beat);
    }
  }
  v_time_to_beat = tempodata_.GetBeatFromTimeArr(v_time);
  v_beat_to_time = tempodata_.GetTimeFromBeatArr(v_beat);
  for (Note& nobj : notedata_)
  {
    if (nobj.pos.type == NotePosTypes::Time)
    {
      nobj.pos.beat = v_time_to_beat[b_idx++];
    } else
    {
      nobj.pos.time_msec = v_beat_to_time[t_idx++];
    }
  }
}

void Chart::InvalidateNotePos(Note &nobj)
{
  if (nobj.pos.type == NotePosTypes::Row)
    nobj.pos.beat = nobj.pos.row.measure + nobj.pos.row.num / (double)nobj.pos.row.deno;
  switch (nobj.pos.type)
  {
    case NotePosTypes::Time:
      nobj.pos.beat = tempodata_.GetBeatFromTime(nobj.pos.time_msec);
      break;
    case NotePosTypes::Row:
    case NotePosTypes::Beat:
      nobj.pos.beat = tempodata_.GetTimeFromBeat(nobj.pos.beat);
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
