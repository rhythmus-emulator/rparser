#include "Chart.h"
#include "rutil.h"

using namespace rutil;

namespace rparser
{

namespace chart
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

std::string Note::toString()
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


Chart::Chart(const MetaData *md, const TempoData *td)
  : metadata_shared_(md), tempodata_shared_(td)
{
}

Chart::Chart(const Chart &nd)
{
  Chart(&nd.GetSharedMetaData(), &nd.GetSharedTempoData());
  for (auto note : nd.notedata_)
  {
    notedata_.push_back(Note(note));
  }
  for (auto stmt : nd.stmtdata_)
  {
    stmtdata_.push_back(ConditionStatement(stmt));
  }
  for (auto action : nd.actiondata_)
  {
    actiondata_.push_back(Action(action));
  }
}

Chart::~Chart()
{
}

void Chart::swap(Chart& c)
{
  notedata_.swap(c.notedata_);
  actiondata_.swap(c.actiondata_);
  stmtdata_.swap(c.stmtdata_);
  tempodata_.swap(c.tempodata_);
  metadata_.swap(c.metadata_);
}


void Chart::Clear()
{
  notedata_.clear();
  actiondata_.clear();
  stmtdata_.clear();
  tempodata_.clear();
  metadata_.Clear();
}

void Chart::Merge(const Chart &cd, rowid_t rowFrom)
{
  notedata_.insert(notedata_.end(), cd.notedata_.begin(), cd.notedata_.end());
  actiondata_.insert(actiondata_.end(), cd.actiondata_.begin(), cd.actiondata_.end());
}

void Chart::AppendStmt(ConditionStatement& stmt)
{
  stmtdata_.push_back(stmt);
}

void Chart::EvaluateStmt(int seed)
{
  for (ConditionStatement stmt : stmtdata_)
  {
    Chart *c = stmt.EvaluateSentence(seed);
    if (c)
      Merge(*c);
  }
}

NoteData& Chart::GetNoteData()
{
  return notedata_;
}

ActionData& Chart::GetActionData()
{
  return actiondata_;
}

TempoData& Chart::GetTempoData()
{
  return tempodata_;
}

MetaData& Chart::GetMetaData()
{
  return metadata_;
}

const NoteData& Chart::GetNoteData() const
{
  return notedata_;
}

const ActionData& Chart::GetActionData() const
{
  return actiondata_;
}

const TempoData& Chart::GetTempoData() const
{
  return tempodata_;
}

const MetaData& Chart::GetMetaData() const
{
  return metadata_;
}

const TempoData& Chart::GetSharedTempoData() const
{
  return tempodata_;
}

const MetaData& Chart::GetSharedMetaData() const
{
  return metadata_;
}

void ConditionStatement::AddSentence(unsigned int cond, Chart* chartdata)
{
  sentences_[cond] = chartdata;
}

Chart* ConditionStatement::EvaluateSentence(int seed)
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
  for (auto p : sentences_)
  {
    delete p.second;
  }
}

MixingData::MixingData() : metadata_alloced_(0)
{
  timingdata_ = new TimingData();
  metadata_ = new MetaData();
}

MixingData::MixingData(const Chart& c, bool deepcopy)
  : metadata_alloced_(0)
{
  MixingNote mn;

  timingdata_ = new TimingData();
  metadata_ = &c.GetMetaData();
  FillTimingData(*timingdata_, c);
  for (auto n : c.GetNotes())
  {
    // TODO: fill more note data
    mn.n = &n;
    mixingnotedata_.push_back(mn);
  }
  if (deepcopy)
  {
    Note *n;
    metadata_ = metadata_alloced_ = new MetaData(metadata_);
    for (auto mn : mixingnotedata_)
    {
      n = new Note(mn.n->second);
      notedata_alloced_.push_back(n);
      mn.n = n;
    }
  }
  // TODO: fill more note data (timing based)
}

std::vector<MixingNote>& MixingData::GetNotes()
{
  return mixingnotedata_;
}

const TimingData& MixingData::GetTimingdata() const
{
  return *timingdata_;
}

const MetaData& MixingData::GetMetadata() const
{
  return *metadata_;
}

MixingData::~MixingData()
{
  for (auto n : notedata_alloced_)
    delete n;
  delete metadata_alloced_;
  delete timingdata_;
}

} /* namespace chart */

} /* namespace rparser */
