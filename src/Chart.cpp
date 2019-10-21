#include "Chart.h"
#include "ChartUtil.h"
#include "Song.h"
#include "rutil.h"

using namespace rutil;

namespace rparser
{

Chart::Chart() : parent_song_(nullptr)
{
}

/*
 * This copy constructor should be used when making duplicated chart
 * So, set same parent as original one.
 */
Chart::Chart(const Chart &nd)
  : parent_song_(nd.parent_song_)
{
  bgmdata_.CopyAll(nd.bgmdata_);
  bgadata_.CopyAll(nd.bgadata_);
  notedata_.CopyAll(nd.notedata_);
  effectdata_.CopyAll(nd.effectdata_);
  metadata_ = nd.metadata_;
  timingsegmentdata_
    = std::move(TimingSegmentData(nd.timingsegmentdata_));
}

Chart::~Chart()
{
}

void Chart::swap(Chart& c)
{
  bgmdata_.swap(c.bgmdata_);
  bgadata_.swap(c.bgadata_);
  notedata_.swap(c.notedata_);
  effectdata_.swap(c.effectdata_);
  metadata_.swap(c.metadata_);
  timingsegmentdata_.swap(c.timingsegmentdata_);
  hash_.swap(c.hash_);
  filename_.swap(c.filename_);
}


void Chart::Clear()
{
  bgmdata_.clear();
  bgadata_.clear();
  notedata_.clear();
  effectdata_.clear();
  metadata_.clear();
  timingsegmentdata_.clear();
}

BgmData& Chart::GetBgmData() { return bgmdata_; }
BgaData& Chart::GetBgaData() { return bgadata_; }
NoteData& Chart::GetNoteData() { return notedata_; }
EffectData& Chart::GetEffectData() { return effectdata_; }
TimingData& Chart::GetTimingData() { return timingsegmentdata_.GetTimingData(); }
TimingSegmentData& Chart::GetTimingSegmentData() { return timingsegmentdata_; }
MetaData& Chart::GetMetaData() { return metadata_; }

const BgmData& Chart::GetBgmData() const { return bgmdata_; }
const BgaData& Chart::GetBgaData() const { return bgadata_; }
const NoteData& Chart::GetNoteData() const { return notedata_; }
const EffectData& Chart::GetEffectData() const { return effectdata_; }
const TimingData& Chart::GetTimingData() const { return timingsegmentdata_.GetTimingData(); }
const TimingSegmentData& Chart::GetTimingSegmentData() const { return timingsegmentdata_; }
const MetaData& Chart::GetMetaData() const { return metadata_; }

uint32_t Chart::GetScoreableNoteCount() const
{
  auto all_track_iter = notedata_.GetAllTrackIterator();
  unsigned cnt = 0;
  while (!all_track_iter.is_end())
  {
    //if (n.IsScoreable())
    ++cnt;
    ++all_track_iter;
  }
  return cnt;
}

double Chart::GetSongLastObjectTime() const
{
  return notedata_.back()->time_msec;
}

bool Chart::HasLongnote() const
{
  auto all_track_iter = notedata_.GetAllTrackIterator();
  while (!all_track_iter.is_end())
  {
    Note &n = static_cast<Note&>(*all_track_iter);
    if (n.chainsize() > 1)
      return true;
  }
  return false;
}

uint8_t Chart::GetPlayLaneCount() const
{
  return notedata_.get_track_count();
}

template <typename T>
void InvalidateTrackDataTiming(T& td, const TimingSegmentData& tsd)
{
  std::vector<NotePos*> vNpos;
  std::vector<double> vBeat, vTime;
  size_t i = 0;
  auto iter = td.GetAllTrackIterator();
  while (!iter.is_end())
  {
    vNpos.push_back(&(*iter));
  }
  for (auto *npos : vNpos)
  {
    vBeat.push_back(npos->beat);
  }
  vTime = std::move(tsd.GetTimeFromBeatArr(vBeat));
  for (auto *npos : vNpos)
  {
    npos->SetTime(vTime[i++]);
  }
}

void Chart::InvalidateAllNotePos()
{
  InvalidateTrackDataTiming(bgmdata_, timingsegmentdata_);
  InvalidateTrackDataTiming(bgadata_, timingsegmentdata_);
  InvalidateTrackDataTiming(notedata_, timingsegmentdata_);
  InvalidateTrackDataTiming(effectdata_, timingsegmentdata_);
}

void Chart::InvalidateNotePos(Note &nobj)
{
  nobj.time_msec = timingsegmentdata_.GetTimeFromBeat(nobj.beat);
}

void Chart::InvalidateTempoData()
{
  timingsegmentdata_.Invalidate(GetMetaData());
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
  ss << "[TempoData]\n" << timingsegmentdata_.toString() << std::endl;
  ss << "[MetaData]\n" << metadata_.toString() << std::endl;
  return ss.str();
}

bool Chart::IsEmpty()
{
  return notedata_.is_empty();
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

void Chart::SetParent(Song *song)
{
  parent_song_ = song;
}

SONGTYPE Chart::GetSongType() const
{
  if (parent_song_)
    return parent_song_->GetSongType();
  else return SONGTYPE::NONE;
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

int ChartList::GetChartIndexByName(const std::string& filename)
{
  int i = 0;
  for (auto *c : charts_)
  {
    if (c->GetFilename() == filename)
      return i;
    i++;
  }
  return -1;
}

ChartNoteList::ChartNoteList() : cur_edit_idx(-1) {}

ChartNoteList::~ChartNoteList() { }

size_t ChartNoteList::size()
{
  return note_charts_.size();
}

int ChartNoteList::AddNewChart()
{
  note_charts_.emplace_back(NoteData());
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

int ChartNoteList::GetChartIndexByName(const std::string& filename)
{
  // not supported.
  return -1;
}


} /* namespace rparser */
