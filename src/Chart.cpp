#include "Chart.h"
#include "ChartUtil.h"
#include "Song.h"
#include "rutil.h"

using namespace rutil;

namespace rparser
{

Chart::Chart() : parent_song_(nullptr), charttype_(CHARTTYPE::None)
{
  bgmdata_.set_track_count(128);
  bgadata_.set_track_count(128);
  effectdata_.set_track_count(128 /* ?? */);
  timingsegmentdata_.GetTimingData().set_track_count(128 /* XXX: use kTimingTypeCount */);
  memset(&common_data_, 0, sizeof(common_data_));
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
  memcpy(&common_data_, &nd.common_data_, sizeof(common_data_));
  charttype_ = nd.charttype_;
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
  std::swap(common_data_, c.common_data_);
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

BgmData& Chart::GetBgmData() { return common_data_.bgmdata_ ? *common_data_.bgmdata_ : bgmdata_; }
BgaData& Chart::GetBgaData() { return common_data_.bgadata_ ? *common_data_.bgadata_ : bgadata_; }
NoteData& Chart::GetNoteData() { return notedata_; }
EffectData& Chart::GetEffectData() { return common_data_.effectdata_ ? *common_data_.effectdata_ : effectdata_; }
TimingData& Chart::GetTimingData()
{ return common_data_.timingsegmentdata_ ? common_data_.timingsegmentdata_->GetTimingData() : timingsegmentdata_.GetTimingData(); }
TimingSegmentData& Chart::GetTimingSegmentData()
{ return common_data_.timingsegmentdata_ ? *common_data_.timingsegmentdata_ : timingsegmentdata_; }
MetaData& Chart::GetMetaData() { return metadata_; }

const BgmData& Chart::GetBgmData() const { return common_data_.bgmdata_ ? *common_data_.bgmdata_ : bgmdata_; }
const BgaData& Chart::GetBgaData() const { return common_data_.bgadata_ ? *common_data_.bgadata_ : bgadata_; }
const NoteData& Chart::GetNoteData() const { return notedata_; }
const EffectData& Chart::GetEffectData() const { return common_data_.effectdata_ ? *common_data_.effectdata_ : effectdata_; }
const TimingData& Chart::GetTimingData() const
{ return common_data_.timingsegmentdata_ ? common_data_.timingsegmentdata_->GetTimingData() : timingsegmentdata_.GetTimingData(); }
const TimingSegmentData& Chart::GetTimingSegmentData() const
{ return common_data_.timingsegmentdata_ ? *common_data_.timingsegmentdata_ : timingsegmentdata_ ; }
const MetaData& Chart::GetMetaData() const { return metadata_; }

uint32_t Chart::GetScoreableNoteCount() const
{
#if 0
  auto all_track_iter = notedata_.GetAllTrackIterator();
  unsigned cnt = 0;
  while (!all_track_iter.is_end())
  {
    //if (n.IsScoreable())
    ++cnt;
    ++all_track_iter;
  }
  return cnt;
#endif
  return notedata_.size();
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
    ++all_track_iter;
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
  std::vector<double> vM, vTime;
  size_t i = 0;
  auto iter = td.GetAllTrackIterator();
  while (!iter.is_end())
  {
    vNpos.push_back(&(*iter));
    ++iter;
  }
  for (auto *npos : vNpos)
  {
    vM.push_back(npos->measure);
  }
  vTime = std::move(tsd.GetTimeFromMeasureArr(vM));
  for (auto *npos : vNpos)
  {
    npos->SetTime(vTime[i++]);
  }
}

void Chart::InvalidateAllNotePos()
{
  InvalidateTrackDataTiming(GetBgmData(), GetTimingSegmentData());
  InvalidateTrackDataTiming(GetBgaData(), GetTimingSegmentData());
  InvalidateTrackDataTiming(GetNoteData(), GetTimingSegmentData());
  InvalidateTrackDataTiming(GetEffectData(), GetTimingSegmentData());
}

void Chart::InvalidateNotePos(Note &nobj)
{
  nobj.time_msec = GetTimingSegmentData().GetTimeFromMeasure(nobj.measure);
}

void Chart::InvalidateTempoData()
{
  GetTimingSegmentData().Invalidate(GetMetaData());
}

void Chart::InvalidateCharttype()
{
  charttype_ = CHARTTYPE::None;
  if (!parent_song_)
    return; /* XXX: manually set SONGTYPE to set proper charttype? */
  int key = notedata_.get_track_count();
  switch (parent_song_->GetSongType())
  {
  case SONGTYPE::BMS:
  case SONGTYPE::BMSON:
    if (key <= 6)
      charttype_ = CHARTTYPE::IIDX5Key;
    else if (key <= 8)
      charttype_ = CHARTTYPE::IIDXSP;
    else if (key <= 10)
      charttype_ = CHARTTYPE::IIDX10Key;
    else
      charttype_ = CHARTTYPE::IIDXDP;
    break;
  case SONGTYPE::PMS:
    charttype_ = CHARTTYPE::Popn;
    break;
  }
}

void Chart::Invalidate()
{
  metadata_.SetMetaFromAttribute();
  metadata_.SetUtf8Encoding();
  InvalidateTempoData();
  InvalidateAllNotePos();
  InvalidateCharttype();
}

std::string Chart::toString() const
{
  std::stringstream ss;
  ss << "[NoteData]\n" << notedata_.toString() << std::endl;
  ss << "[TempoData]\n" << GetTimingSegmentData().toString() << std::endl;
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

Song* Chart::GetParent() const
{
  return parent_song_;
}

CHARTTYPE Chart::GetChartType() const
{
  return charttype_;
}

int Chart::GetKeycount() const
{
  switch (GetChartType())
  {
  case CHARTTYPE::Chart4Key:
  case CHARTTYPE::DDR:
    return 4;
  case CHARTTYPE::Chart5Key:
  case CHARTTYPE::IIDX5Key:
  case CHARTTYPE::Pump:
    return 5;
  case CHARTTYPE::Chart6Key:
    return 6;
  case CHARTTYPE::Chart7Key:
  case CHARTTYPE::IIDXSP:
    return 7;
  case CHARTTYPE::Chart8Key:
  case CHARTTYPE::DDR_DP:
    return 8;
  case CHARTTYPE::Chart9Key:
    return 9;
  case CHARTTYPE::Chart10Key:
  case CHARTTYPE::IIDX10Key:
  case CHARTTYPE::Pump_DP:
    return 10;
  case CHARTTYPE::IIDXDP:
    return 14;
  case CHARTTYPE::None:
  default:
    return -1;
  }
}

} /* namespace rparser */
