#include "Chart.h"
#include "ChartUtil.h"
#include "Song.h"
#include "rutil.h"
#include "common.h"

using namespace rutil;

namespace rparser
{

Chart::Chart() : parent_song_(nullptr), charttype_(CHARTTYPE::None)
{
  trackdata_[TrackTypes::kTrackTiming].set_track_count(TimingTrackTypes::kTimingTrackMax);
  trackdata_[TrackTypes::kTrackTap].set_track_count(128);
  trackdata_[TrackTypes::kTrackCommand].set_track_count(128);
  trackdata_[TrackTypes::kTrackBGM].set_track_count(128);

  memset(&shared_data_, 0, sizeof(shared_data_));
  seed_ = 0;
}

/*
 * This copy constructor should be used when making duplicated chart
 * So, set same parent as original one.
 */
Chart::Chart(const Chart &c)
  : parent_song_(c.parent_song_)
{
  for (size_t i = 0; i < TrackTypes::kTrackMax; ++i)
    trackdata_[i] = c.trackdata_[i];
  metadata_ = c.metadata_;
  timingsegmentdata_
    = std::move(TimingSegmentData(c.timingsegmentdata_));
  memcpy(&shared_data_, &c.shared_data_, sizeof(shared_data_));
  charttype_ = c.charttype_;
  hash_ = c.hash_;
  seed_ = c.seed_;
}

Chart::~Chart()
{
}

void Chart::swap(Chart& c)
{
  for (size_t i = 0; i < TrackTypes::kTrackMax; ++i)
    trackdata_[i].swap(c.trackdata_[i]);
  metadata_.swap(c.metadata_);
  timingsegmentdata_.swap(c.timingsegmentdata_);
  hash_.swap(c.hash_);
  filename_.swap(c.filename_);
  std::swap(shared_data_, c.shared_data_);
}


void Chart::Clear()
{
  for (size_t i = 0; i < TrackTypes::kTrackMax; ++i)
    trackdata_[i].clear();
  metadata_.clear();
  timingsegmentdata_.clear();
}

TrackData& Chart::GetBgmData() { return trackdata_[TrackTypes::kTrackBGM]; }
TrackData& Chart::GetNoteData() { return trackdata_[TrackTypes::kTrackTap]; }
TrackData& Chart::GetCommandData() {
  return shared_data_.trackdata[TrackTypes::kTrackCommand]
    ? *shared_data_.trackdata[TrackTypes::kTrackCommand]
    : trackdata_[TrackTypes::kTrackCommand];
}
TrackData& Chart::GetTimingData() {
  return shared_data_.trackdata[TrackTypes::kTrackTiming]
    ? *shared_data_.trackdata[TrackTypes::kTrackTiming]
    : trackdata_[TrackTypes::kTrackTiming];
}
TimingSegmentData& Chart::GetTimingSegmentData() {
  return shared_data_.timingsegmentdata
    ? *shared_data_.timingsegmentdata
    : timingsegmentdata_;
}
MetaData& Chart::GetMetaData() { return metadata_; }

const TrackData& Chart::GetBgmData() const { return trackdata_[TrackTypes::kTrackBGM]; }
const TrackData& Chart::GetNoteData() const { return trackdata_[TrackTypes::kTrackTap]; }
const TrackData& Chart::GetCommandData() const  {
  return shared_data_.trackdata[TrackTypes::kTrackCommand]
    ? *shared_data_.trackdata[TrackTypes::kTrackCommand]
    : trackdata_[TrackTypes::kTrackCommand];
}
const TrackData& Chart::GetTimingData() const {
  return shared_data_.trackdata[TrackTypes::kTrackTiming]
    ? *shared_data_.trackdata[TrackTypes::kTrackTiming]
    : trackdata_[TrackTypes::kTrackTiming];
}
const TimingSegmentData& Chart::GetTimingSegmentData() const {
  return shared_data_.timingsegmentdata
    ? *shared_data_.timingsegmentdata
    : timingsegmentdata_;
}
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
  return GetNoteData().GetNoteCount();
}

double Chart::GetSongLastObjectTime() const
{
  return GetNoteData().back()->time();
}

bool Chart::HasLongnote() const
{
  return GetNoteData().HasLongnote();
}

unsigned Chart::GetPlayLaneCount() const
{
  return (unsigned)GetNoteData().get_track_count();
}

void InvalidateTrackDataTiming(TrackData& td, const TimingSegmentData& tsd)
{
  size_t i = 0, p1 = 0, p2 = 0;
  double t = 0;
  auto rows = RowCollection(td);
  for (auto &row : rows) {
    t = tsd.GetTimeFromMeasure(row.pos, p1, p2);
    for (auto &p : row.notes) {
      auto* note = p.second;
      note->set_time(t);
    }
  }
}

void Chart::UpdateAllNotePos()
{
  InvalidateTrackDataTiming(GetBgmData(), GetTimingSegmentData());
  InvalidateTrackDataTiming(GetNoteData(), GetTimingSegmentData());
  InvalidateTrackDataTiming(GetCommandData(), GetTimingSegmentData());
}

void Chart::UpdateNotePos(NoteElement &nobj)
{
  nobj.set_time( GetTimingSegmentData().GetTimeFromMeasure(nobj.measure()) );
}

void Chart::UpdateTempoData()
{
  GetTimingSegmentData().Update(&GetMetaData(), GetTimingData());
}

void Chart::UpdateCharttype()
{
  charttype_ = CHARTTYPE::None;
  if (!parent_song_)
    return; /* XXX: manually set SONGTYPE to set proper charttype? */
  int key = GetNoteData().get_track_count();
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

void Chart::Update()
{
  metadata_.SetMetaFromAttribute();
  metadata_.SetUtf8Encoding();
  UpdateTempoData();
  UpdateAllNotePos();
  UpdateCharttype();
}

std::string Chart::toString() const
{
  std::stringstream ss;
  ss << "[TimingData]\n" << trackdata_[TrackTypes::kTrackTiming].toString() << std::endl;
  ss << "[NoteData]\n" << trackdata_[TrackTypes::kTrackTap].toString() << std::endl;
  ss << "[CommandData]\n" << trackdata_[TrackTypes::kTrackCommand].toString() << std::endl;
  ss << "[BgmData]\n" << trackdata_[TrackTypes::kTrackBGM].toString() << std::endl;
  ss << "[TimingSegment]\n" << GetTimingSegmentData().toString() << std::endl;
  ss << "[MetaData]\n" << metadata_.toString() << std::endl;
  return ss.str();
}

bool Chart::IsEmpty() const
{
  return GetNoteData().is_empty();
}

const std::string &Chart::GetHash() const
{
  if (hash_.empty() && !IsEmpty()) {
    std::string nd_serialized = GetNoteData().Serialize();
    hash_ = md5_str(nd_serialized.c_str(), nd_serialized.size());
  }
  return hash_;
}

int Chart::GetSeed() const
{
  return seed_;
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
