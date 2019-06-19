#include "Chart.h"
#include "ChartUtil.h"
#include "rutil.h"

using namespace rutil;

namespace rparser
{

Chart::Chart()
{
}

Chart::Chart(const Chart &nd)
{
  for (const auto& note : nd.notedata_)
  {
    notedata_.AddNote(SoundNote(note));
  }
  for (const auto& note : nd.cmddata_)
  {
    cmddata_.AddNote(EventNote(note));
  }
  for (const auto& stmt : nd.stmtdata_)
  {
    stmtdata_.push_back(ConditionalChart(stmt));
  }
  tempodata_ = std::move(TempoData(nd.tempodata_));
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

ConditionalChart* Chart::CreateStmt(int value, bool israndom, bool isswitchstmt)
{
  stmtdata_.emplace_back(ConditionalChart(value, israndom, isswitchstmt));
  return GetLastStmt();
}

ConditionalChart* Chart::GetLastStmt()
{
  return &stmtdata_.back();
}

void Chart::AppendStmt(ConditionalChart& stmt)
{
  stmtdata_.push_back(stmt);
}

void Chart::EvaluateStmt(rutil::Random &random)
{
  for (const ConditionalChart& stmt : stmtdata_)
  {
    Chart *c = stmt.EvaluateSentence(random);
    if (c)
    {
      notedata_.Merge(c->GetNoteData());
      GetTempoData().GetTempoNoteData().Merge(c->GetTempoData().GetTempoNoteData());
      GetMetaData().MergeAttributes(c->GetMetaData());
    }
  }
}

NoteData<SoundNote>& Chart::GetNoteData()
{
  return notedata_;
}

NoteData<EventNote>& Chart::GetEventNoteData()
{
  return cmddata_;
}

MetaData& Chart::GetMetaData()
{
  return metadata_;
}

TempoData& Chart::GetTempoData()
{
  return tempodata_;
}

const NoteData<SoundNote>& Chart::GetNoteData() const
{
  return notedata_;
}

const NoteData<EventNote>& Chart::GetEventNoteData() const
{
  return cmddata_;
}

const TempoData& Chart::GetTempoData() const
{
  return tempodata_;
}

const MetaData& Chart::GetMetaData() const
{
  return metadata_;
}

uint32_t Chart::GetScoreableNoteCount() const
{
  const auto &nd = GetNoteData();
  uint32_t cnt = 0;
  for (auto &n : nd)
    if (n.IsScoreable()) cnt++;
  return cnt;
}

double Chart::GetSongLastObjectTime() const
{
  return GetNoteData().back().GetTimePos();
}

double Chart::GetSongLastScorableObjectTime() const
{
  const auto &nd = GetNoteData();
  for (auto i = nd.rbegin(); i != nd.rend(); ++i)
  {
    if (i->IsScoreable())
      return i->GetTimePos();
  }
  return 0;
}

bool Chart::HasLongnote() const
{
  for (auto &n : GetNoteData())
    if (n.IsLongnote()) return true;
  return false;
}

template<typename N>
void InvalidateNoteDataPos(NoteData<N>& nd, const TempoData& tempodata_)
{
  std::vector<double> v_time, v_beat, v_row;
  int t_idx = 0, b_idx = 0, r_idx = 0;
  // Sort by row / beat / time.
  SortedNoteObjects sorted;
  SortNoteObjectsByType(nd, sorted);
  // Make conversion of row --> beat --> time first.
  for (const Note* nobj : sorted.nobj_by_row)
    v_row.push_back(nobj->pos().measure);
  const std::vector<double> v_row_to_beat(std::move(tempodata_.GetBeatFromBarArr(v_row)));
  for (Note* nobj : sorted.nobj_by_row)
    v_beat.push_back(nobj->pos().beat = v_row_to_beat[r_idx++]);
  const std::vector<double> v_row_to_time(std::move(tempodata_.GetTimeFromBeatArr(v_beat)));
  r_idx = 0;
  for (Note* nobj : sorted.nobj_by_row)
    nobj->pos().time_msec = v_row_to_time[r_idx++];
  v_beat.clear();
  // Make conversion of beat <--> time.
  for (const Note* nobj : sorted.nobj_by_beat)
    v_beat.push_back(nobj->pos().beat);
  for (const Note* nobj : sorted.nobj_by_tempo)
    v_time.push_back(nobj->pos().time_msec);
  const std::vector<double>&& v_time_to_beat = tempodata_.GetBeatFromTimeArr(v_time);
  const std::vector<double>&& v_beat_to_time = tempodata_.GetTimeFromBeatArr(v_beat);
  for (Note* nobj : sorted.nobj_by_beat)
    nobj->pos().time_msec = v_beat_to_time[t_idx++];
  for (Note* nobj : sorted.nobj_by_tempo)
    nobj->pos().beat = v_time_to_beat[b_idx++];
  // Fill beat --> row if beat pos type
  const std::vector<double>&& v_beat_to_row = tempodata_.GetMeasureFromBeatArr(v_beat);
  t_idx = 0;
  for (Note* nobj : sorted.nobj_by_beat)
    nobj->pos().measure = v_beat_to_row[t_idx++];
}

void Chart::InvalidateAllNotePos()
{
  InvalidateNoteDataPos(notedata_, tempodata_);
  InvalidateNoteDataPos(cmddata_, tempodata_);

  // for longnote ... FIXME it'll may cause rather bad performance.
  for (auto &n : notedata_)
  {
    if (n.IsLongnote())
    {
      for (auto &chain : n.chains)
      {
        switch (chain.pos.postype())
        {
        case NotePosTypes::Bar:
          chain.pos.beat = tempodata_.GetBeatFromRow(chain.pos.measure);
        case NotePosTypes::Beat:
          chain.pos.time_msec = tempodata_.GetTimeFromBeat(chain.pos.beat);
          break;
        case NotePosTypes::Time:
          chain.pos.beat = tempodata_.GetBeatFromTime(chain.pos.time_msec);
          break;
        }
      }
    }
  }
}

void Chart::InvalidateNotePos(Note &nobj)
{
  NotePos &npos = nobj.pos();
  switch (npos.type)
  {
    case NotePosTypes::Time:
      npos.beat = tempodata_.GetBeatFromTime(npos.time_msec);
      break;
    case NotePosTypes::Bar:
      npos.beat = tempodata_.GetBeatFromRow(npos.measure);
      npos.time_msec = tempodata_.GetTimeFromBeat(npos.beat);
      break;
    case NotePosTypes::Beat:
      npos.time_msec = tempodata_.GetTimeFromBeat(npos.beat);
      npos.measure = tempodata_.GetMeasureFromBeat(npos.beat);
      break;
  }
}

void Chart::InvalidateTempoData()
{
  tempodata_.Invalidate(GetMetaData());
}

void Chart::Invalidate()
{
  metadata_.SetMetaFromAttribute();
  metadata_.SetUtf8Encoding();
  InvalidateTempoData();
  InvalidateAllNotePos();
}

std::string Chart::toString() const
{
  std::stringstream ss;
  ss << "[NoteData]\n" << notedata_.toString() << std::endl;
  ss << "[TempoData]\n" << tempodata_.toString() << std::endl;
  ss << "[MetaData]\n" << metadata_.toString() << std::endl;
  return ss.str();
}

bool Chart::IsEmpty()
{
  return notedata_.IsEmpty();
}

std::string Chart::GetHash() const
{
  return hash_;
}

void Chart::SetHash(const std::string& hash)
{
  hash_ = hash;
}

std::string Chart::GetFilename() const
{
  return filename_;
}

void Chart::SetFilename(const std::string& filename)
{
  filename_ = filename;
}

void ConditionalChart::AddSentence(unsigned int cond, Chart* chartdata)
{
  sentences_[cond] = chartdata;
}

Chart* ConditionalChart::EvaluateSentence(rutil::Random& random) const
{
  uint32_t idx = static_cast<uint32_t>(random.Next()) % value_;
  auto it = sentences_.find(idx);
  if (it == sentences_.end())
    return 0;
  return it->second;
}

size_t ConditionalChart::GetSentenceCount()
{
  return sentences_.size();
}

Chart* ConditionalChart::CreateSentence(unsigned int cond)
{
  auto ii = sentences_.find(cond);
  if (ii == sentences_.end())
    sentences_.emplace(cond, new Chart());
  return sentences_[cond];
}

ConditionalChart::ConditionalChart(int value, bool israndom, bool isswitchstmt)
  : value_(value), israndom_(israndom), isswitchstmt_(isswitchstmt)
{
  assert(value_ > 0);
}

ConditionalChart::ConditionalChart(const ConditionalChart& cs)
  : value_(cs.value_), israndom_(cs.israndom_), isswitchstmt_(cs.isswitchstmt_)
{
  for (auto p : cs.sentences_)
  {
    sentences_[p.first] = new Chart(*p.second);
  }
}

ConditionalChart::~ConditionalChart()
{
  for (auto& p : sentences_)
  {
    delete p.second;
  }
}

ChartList::ChartList() : cur_edit_idx(-1) {}

ChartList::~ChartList()
{
  for (auto c : charts_)
  {
    delete c;
  }
}

size_t ChartList::size()
{
  return charts_.size();
}

int ChartList::AddNewChart()
{
  charts_.push_back(new Chart());
  return charts_.size() - 1;
}

const Chart* ChartList::GetChartData(int idx) const
{
  return const_cast<ChartList*>(this)->GetChartData(idx);
}

Chart* ChartList::GetChartData(int idx)
{
  if (idx < 0 || idx >= (int)charts_.size() || cur_edit_idx >= 0) return 0;
  cur_edit_idx = idx;
  return charts_[idx];
}

void ChartList::CloseChartData()
{
  cur_edit_idx = -1;
}

void ChartList::DeleteChart(int idx)
{
  if (idx < 0 || idx >= (int)charts_.size() || cur_edit_idx == idx) return;
  auto ii = charts_.begin() + idx;
  delete *ii;
  charts_.erase(ii);
}

void ChartList::UpdateTempoData()
{
  for (auto c : charts_)
    c->InvalidateTempoData();
}

void ChartList::AppendChart(Chart* chart)
{
  charts_.push_back(chart);
}

Chart* ChartList::GetChart(int idx)
{
  if (idx < 0 || idx > (int)charts_.size()) return 0;
  return charts_[idx];
}

bool ChartList::IsChartOpened()
{
  return cur_edit_idx >= 0;
}

ChartNoteList::ChartNoteList() : cur_edit_idx(-1) {}

ChartNoteList::~ChartNoteList() { }

size_t ChartNoteList::size()
{
  return note_charts_.size();
}

int ChartNoteList::AddNewChart()
{
  note_charts_.push_back(NoteData<SoundNote>());
  return note_charts_.size() - 1;
}

const Chart* ChartNoteList::GetChartData(int idx) const
{
  return const_cast<Chart*>(GetChartData(idx));
}

Chart* ChartNoteList::GetChartData(int idx)
{
  if (idx < 0 || idx >= (int)note_charts_.size() || cur_edit_idx >= 0) return 0;
  // swap note data context and return self object.
  note_charts_[idx].swap(GetNoteData());
  cur_edit_idx = idx;
  return this;
}

void ChartNoteList::CloseChartData()
{
  if (cur_edit_idx >= 0)
  {
    // re-swap note data context.
    std::swap(GetNoteData(), note_charts_[cur_edit_idx]);
    cur_edit_idx = -1;
  }
}

void ChartNoteList::DeleteChart(int idx)
{
  if (idx < 0 || idx >= (int)note_charts_.size() || cur_edit_idx == idx) return;
  note_charts_.erase(note_charts_.begin() + idx);
}

bool ChartNoteList::IsChartOpened()
{
  return cur_edit_idx >= 0;
}

void ChartNoteList::UpdateTempoData()
{
  InvalidateTempoData();
}


} /* namespace rparser */
