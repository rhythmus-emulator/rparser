#include "Chart.h"
#include "rutil.h"

using namespace rutil;

namespace rparser
{

const char *n_type[] = {
  "NOTE_EMPTY",
  "NOTE_TAP",
  "NOTE_TOUCH",
  "NOTE_VEFX",
  "NOTE_BGM",
  "NOTE_BGA",
  "NOTE_MIDI",
  "NOTE_BMS",
  "NOTE_REST",
  "NOTE_SHOCK",
};

const char *n_subtype_tap[] = {
  "TAPNOTE_EMPTY",
  "TAPNOTE_TAP",
  "TAPNOTE_INVISIBLE",
  "TAPNOTE_CHARGE",
  "TAPNOTE_HCHARGE",
  "TAPNOTE_TCHARGE",
  "TAPNOTE_MINE",
  "TAPNOTE_AUTOPLAY",
  "TAPNOTE_FAKE",
  "TAPNOTE_EXTRA",
  "TAPNOTE_FREE",
};

const char *n_subtype_bga[] = {
  "NOTEBGA_EMPTY",
  "NOTEBGA_BGA",
  "NOTEBGA_MISS",
  "NOTEBGA_LAYER1",
  "NOTEBGA_LAYER2",
};

const char *n_subtype_bms[] = {
  "NOTEBMS_EMPTY",
  "NOTEBMS_BPM",
  "NOTEBMS_STOP",
  "NOTEBMS_BGAOPAC",
  "NOTEBMS_INVISIBLE",
};

std::string Note::toString() const
{
  std::stringstream ss;
  std::string sType, sSubtype;
  sType = n_type[(int)type];
  switch (nType)
  {
  case NOTETYPE::NOTE_TAP:
  case NOTE_TOUCH:
    sSubtype = n_subtype_tap[subType];
    break;
  case NOTE_BGA:
    sSubtype = n_subtype_bga[subType];
    break;
  case NOTE_BGM:
    sSubtype = "(COL NUMBER)";
    break;
  case NOTE_BMS:
    sSubtype = n_subtype_bms[subType];
    break;
  default:
    sSubtype = "(UNSUPPORTED)";
    break;
  }
  ss << "[Object Note]\n";
  ss << "type/subtype: " << sType << "," << sSubtype <<
    " (" << nType << "," << subType << ")\n";
  ss << "Value / EndValue: " << iValue << "," << iEndValue << std::endl;
  ss << "Beat / BeatLength: " << fBeat << "," << fBeatLength << std::endl;
  ss << "Row / iDuration: " << iRow << "," << iDuration << std::endl;
  ss << "Time / fDuration: " << fTime << "," << fTimeLength << std::endl;
  ss << "x / y: " << x << "," << y << std::endl;
  return ss.str();
}

bool Note::operator<(const Note &other) const
{
  // first compare beat, next track(x).
  return (fBeat == other.fBeat) ? x < other.x : fBeat < other.fBeat;
}
bool Note::operator==(const Note &other) const
{
  return
    nType == other.nType &&
    subType == other.subType &&
    iRow == other.iRow &&
    x == other.x &&
    y == other.y;
}

bool Note::IsTappableNote()
{
  return nType == NoteType::NOTE_TAP;
}
int Note::GetPlayerSide()
{
  return x / 10;
}
int Note::GetTrack()
{
  return x;
}

struct n_less_row : std::binary_function <Note, Note, bool>
{
  bool operator() (const Note& x, const Note& y) const
  {
    return x.iRow < y.iRow;
  }
};

struct nptr_less : std::binary_function <Note*, Note*, bool>
{
  bool operator() (const Note* x, const Note* y) const
  {
    return x->fBeat < y->fBeat;
  }
};

struct nptr_less_row : std::binary_function <Note*, Note*, bool>
{
  bool operator() (const Note* x, const Note* y) const
  {
    return x->iRow < y->iRow;
  }
};


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
