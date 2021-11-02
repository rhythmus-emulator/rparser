#include "TempoData.h"
#include "MetaData.h"
#include "ChartUtil.h"
#include "common.h"
#include <iomanip>

namespace rparser
{

static bool use_detailed_info = false;

inline double GetTimeFromBeatInTempoSegment(const TimingSegment& tobj, double beat)
{
  const double msec_per_beat = 60.0 * 1000 / tobj.bpm_ / tobj.scrollspeed_;
  const double beat_delta = beat - tobj.beat_ - tobj.warpbeat_ /* XXX: warpbeat is not affected by barlength? */;
  if (beat_delta < 0) return tobj.time_ + tobj.stoptime_;
  else return tobj.time_ + tobj.stoptime_ + tobj.delaytime_ + beat_delta * msec_per_beat;
}

inline double GetBeatFromTimeInTempoSegment(const TimingSegment& tobj, double time_msec)
{
  const double beat_per_msec = tobj.bpm_ * tobj.scrollspeed_ / 60 / 1000;
  const double time_delta = time_msec - tobj.time_ - (tobj.stoptime_ + tobj.delaytime_);
  if (time_delta <= 0) return tobj.beat_;
  return tobj.beat_ + tobj.warpbeat_ + time_delta * beat_per_msec;
}

inline double GetBeatFromMeasureInBarSegment(const BarObject& b, double measure, bool recover_length)
{
  double diff = measure - (double)b.measure_;
  if (recover_length && diff > 1)
    return b.beat_ + ((diff - 1) + b.barlength_) * kDefaultMeasureLength;
  else
    return b.beat_ + (diff * b.barlength_) * kDefaultMeasureLength;
}

inline double GetMeasureFromBeatInBarSegment(const BarObject& b, double beat, bool recover_length)
{
  double diff = beat - b.beat_;
  if (recover_length && diff > b.barlength_ * kDefaultMeasureLength)
    return b.measure_ + 1 + (diff - b.barlength_) / kDefaultMeasureLength;
  else
    return b.measure_ + diff / b.barlength_ / kDefaultMeasureLength;
}

inline uint32_t GetMeasureFromBarInBeatSegment(const BarObject& b, double measure, bool recover_length)
{
  return static_cast<uint32_t>(
    GetMeasureFromBarInBeatSegment(b, measure, recover_length)
    + 0.000001 /* for calculation error */
    );
}

TimingSegment::TimingSegment()
: beat_(0), time_(0), measure_(0), bpm_(kDefaultBpm),
  stoptime_(0), delaytime_(0), warpbeat_(0), scrollspeed_(1), tick_(1), is_manipulated_(false) {}

// called when TempoObject is copied for new block (e.g. Seek methods)
// As new object segment won't have same STOP/DELAY/WARP by default.
void TimingSegment::clearForCopiedSegment()
{
  stoptime_ = 0;
  delaytime_ = 0;
  warpbeat_ = 0;
  is_manipulated_ = false;
}

std::string TimingSegment::toString() const
{
  std::stringstream ss;
  ss << std::fixed << std::setprecision(2)
     << "B" << beat_ << "/M" << measure_ << "/T" << time_
     << " - BPM: " << bpm_ << ", STOP: " << stoptime_ << std::endl;
  return ss.str();
}

BarObject::BarObject()
  : measure_(0), barlength_(1.0), beat_(0) {}

BarObject::BarObject(double beat, double barlength, uint32_t measure)
  : measure_(measure), barlength_(barlength), beat_(beat) {}

bool BarObject::operator<(const BarObject &other) const
{
  return measure_ < other.measure_;
}

TimingSegmentData::TimingSegmentData()
{
  clear();
}

int GetSmallestIndex(double arr[], int size)
{
  int r = 0;
  double n = arr[0];
  for (int i = 1; i < size; i++)
    if (n > arr[i]) n = arr[i], r = i;
  return r;
}

// @warn  All objects in track is expected to be valid.
void TimingSegmentData::Update(const MetaData *md, TrackData& timingtrack)
{
  float v;

  // Set first bpm from metadata (if exists)
  if (md)
    SetBPMChange(md->bpm);

  /**
   * COMMENT
   * Bpm channel and Bmsbpm channel object may exist in same place,
   * and it originally produces unknown result, and as it's edge case we can ignore it.
   * But L99^ song total time is much different unless latter object(Bmsbpm) has bpm precedence.
   * So, in shortcut, we do sorting in subtype to make Bmsbpm object has more precedence.
   * -- all_track_iterator automatically do this, so no need to care about it here.
   */

  auto rows = RowCollection(timingtrack);
  double curr_time = 0;

  // Make tempo segments
  for (auto &row : rows) {
    // seek for next tempo segment object and update note object beat value.
    SeekByMeasure(row.pos);
    curr_time = timingsegments_.back().time_;

    for (auto &p : row.notes) {
      auto nelem = p.second;
      switch (p.first) {
      case TimingTrackTypes::kMeasure:
        SetMeasureLengthChange(static_cast<uint32_t>(nelem->measure()), nelem->get_value_f());
        break;
      case TimingTrackTypes::kScroll:
        SetScrollSpeedChange(nelem->get_value_f());
        break;
      case TimingTrackTypes::kBpm:
        SetBPMChange(nelem->get_value_f());
        break;
      case TimingTrackTypes::kBmsBpm:
        RPARSER_ASSERT(md, "Metadata is nullptr while BmsBpm exists...");
        if (md->GetBPMChannel()->GetBpm(nelem->get_value_u(), v))
          SetBPMChange(v);
        else
          RPARSER_LOG("Failed to fetch BPM information from Metadata while TimingSegmentData::Update.");
        break;
      case TimingTrackTypes::kStop:
        SetSTOP(nelem->get_value_f());
        break;
      case TimingTrackTypes::kBmsStop:
        RPARSER_ASSERT(md, "Metadata is nullptr while BmsStop exists...");
        if (md->GetSTOPChannel()->GetStop(nelem->get_value_u(), v))
          SetSTOP(v / 192.0 * 4.0);
        else
          RPARSER_LOG("Failed to fetch STOP information from Metadata while TimingSegmentData::Update.");
        break;
      case TimingTrackTypes::kTick:
        SetTick(nelem->get_value_i());
        break;
      case TimingTrackTypes::kWarp:
        SetWarp(nelem->get_value_f());
        break;
      }
    }
  }
}

double TimingSegmentData::GetTimeFromMeasure(double measure) const
{
  return GetTimeFromBeat(GetBeatFromMeasure(measure));
}

double TimingSegmentData::GetTimeFromMeasure(double measure, size_t &tidx, size_t &bidx) const
{
  // @brief Cache timingsegmentindex + barindex for fast linear search.
  const size_t maxsize = timingsegments_.size();
  for (; tidx < maxsize - 1; ++tidx)
  {
    if (timingsegments_[tidx].measure_ <= measure)
    {
      if (measure < timingsegments_[tidx + 1].measure_)
        return GetTimeFromBeatInTempoSegment(timingsegments_[tidx], GetBeatFromMeasure(measure, bidx));
    }
    else break;
  }
  // if idx is last segment then use it.
  if (tidx == maxsize - 1)
    return GetTimeFromBeatInTempoSegment(timingsegments_[tidx], GetBeatFromMeasure(measure, bidx));
  // if not found proper segment from given index, then start from first segment.
  tidx = 0;
  return GetTimeFromMeasure(measure, tidx, bidx);
}

double TimingSegmentData::GetMeasureFromTime(double time) const
{
  return GetMeasureFromBeat(GetBeatFromTime(time));
}

double TimingSegmentData::GetTimeFromBeat(double beat) const
{
  // binary search to find proper tempoobject segment.
  int min = 0, max = timingsegments_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  while( l <= r )
  {
    int m = ( l + r ) / 2;
    if( ( m == min || timingsegments_[m].beat_ <= beat ) && ( m == max || beat < timingsegments_[m+1].beat_ ) )
    {
      idx = m;
      break;
    }
    else if( timingsegments_[m].beat_ > beat )
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }
  return GetTimeFromBeatInTempoSegment(timingsegments_[idx], beat);
}

double TimingSegmentData::GetBeatFromTime(double time) const
{
  // binary search to find proper tempoobject segment.
  int min = 0, max = timingsegments_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  while( l <= r )
  {
    int m = ( l + r ) / 2;
    if( ( m == min || timingsegments_[m].time_ <= time ) && ( m == max || time < timingsegments_[m+1].time_ ) )
    {
      idx = m;
      break;
    }
    else if( timingsegments_[m].time_ > time )
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }
  return GetBeatFromTimeInTempoSegment(timingsegments_[idx], time);
}

double TimingSegmentData::GetBeatFromMeasure(double m) const
{
  /**
   * NOTE: CANNOT be replaced with measure + num / deno method
   * as we should care about measure length.
   */

  // binary search to find proper tempoobject segment.
  int min = 0, max = barobjs_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  uint32_t measure = static_cast<uint32_t>(floorl(m));
  while( l <= r )
  {
    int m = ( l + r ) / 2;

    if( ( m == min || barobjs_[m].measure_ <= measure) && ( m == max || measure < barobjs_[m+1].measure_) )
    {
      idx = m;
      break;
    }
    else if(barobjs_[m].measure_ > measure)
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }

  return GetBeatFromMeasureInBarSegment(barobjs_[idx], m, do_recover_measure_length_);
}

double TimingSegmentData::GetBeatFromMeasure(double m, size_t &idx) const
{
  // @brief for linear search.
  // (exception case) if negative?
  if (m < 0)
  {
    return -GetBeatFromMeasureInBarSegment(barobjs_[0], -m, do_recover_measure_length_);
  }

  uint32_t measure = static_cast<uint32_t>(floorl(m));
  while (idx < barobjs_.size() && measure >= barobjs_[idx].measure_)
  {
    if (idx + 1 == barobjs_.size() ||
       (idx + 1 < barobjs_.size() && measure < barobjs_[idx + 1].measure_))
    {
      return GetBeatFromMeasureInBarSegment(barobjs_[idx], m, do_recover_measure_length_);
    }
    ++idx;
  }

  // restart from beginning if not found
  idx = 0;
  return GetBeatFromMeasure(m, idx);
}

double TimingSegmentData::GetMeasureFromBeat(double beat) const
{
  // binary search to find proper tempoobject segment.
  int min = 0, max = barobjs_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  while( l <= r )
  {
    int m = ( l + r ) / 2;
    if( ( m == min || barobjs_[m].beat_ <= beat ) && ( m == max || beat < barobjs_[m+1].beat_) )
    {
      idx = m;
      break;
    }
    else if(barobjs_[m].beat_ > beat )
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }
  return GetMeasureFromBeatInBarSegment(barobjs_[idx], beat, do_recover_measure_length_);
}

std::vector<double> TimingSegmentData::GetTimeFromMeasureArr(const std::vector<double>& sorted_measure) const
{
  std::vector<double> r_beat, r_time;
  size_t idx = 0;
  r_beat = std::move(GetBeatFromMeasureArr(sorted_measure));

  idx = 0;
  for (double beat: r_beat)
  {
    // go to next segment if available.
    if (idx+1 < timingsegments_.size() && beat > timingsegments_[idx+1].beat_) idx++;
    r_time.push_back(GetTimeFromBeatInTempoSegment(timingsegments_[idx], beat));
  }
  return r_time;
}

std::vector<double> TimingSegmentData::GetMeasureFromTimeArr(const std::vector<double>& sorted_time) const
{
  std::vector<double> r_beat, r_measure;
  size_t idx = 0;
  for (double time: sorted_time)
  {
    // go to next segment if available.
    if (idx+1 < timingsegments_.size() && time > timingsegments_[idx+1].time_) idx++;
    r_beat.push_back(GetBeatFromTimeInTempoSegment(timingsegments_[idx], time));
  }

  idx = 0;
  for (double beat : r_beat)
  {
    if (idx + 1 < barobjs_.size() && beat > barobjs_[idx + 1].beat_) idx++;
    r_measure.push_back(GetMeasureFromBeatInBarSegment(barobjs_[idx], beat, do_recover_measure_length_));
  }
  return r_measure;
}

std::vector<double> TimingSegmentData::GetBeatFromMeasureArr(const std::vector<double>& sorted_measure) const
{
  std::vector<double> r_beat;
  size_t idx = 0;
  for (double m: sorted_measure)
  {
    // go to next segment if available.
    // -- measure pos might be same, so move as much as possible.
    while (idx+1 < barobjs_.size() && m > (double)barobjs_[idx+1].measure_)
      idx++;
    r_beat.push_back(GetBeatFromMeasureInBarSegment(barobjs_[idx], m, do_recover_measure_length_));
  }
  return r_beat;
}

void TimingSegmentData::SetFirstObjectFromMetaData(const MetaData &md)
{
  TimingSegment &o = timingsegments_.front();
  o.bpm_ = md.bpm;
}

void TimingSegmentData::SetMeasureLengthChange(uint32_t measure_idx, double barlength)
{
  // don't do unnecessary thing as much as possible
  if (barlength <= 0)
  {
    std::cerr << "[Warning] measure length below zero ignored." << std::endl;
    return;
  }
  // Only use last segment.
  BarObject& b = barobjs_.back();
  ASSERT(measure_idx >= b.measure_); /* same bar editing with do_recover_measure_length */
  if (b.barlength_ == barlength && measure_idx == b.measure_) return;

  // If same measure_idx, then update previous one.
  if (measure_idx == b.measure_)
  {
    barobjs_.back().barlength_ = barlength;
    return;
  }

  // Set bar segment
  BarObject new_obj(b);
  uint32_t measure_count = measure_idx - b.measure_;
  uint32_t defmeasure_count = 0;
  if (do_recover_measure_length_)
  {
    defmeasure_count = measure_count - 1;
    measure_count = 1;
  }
  new_obj.beat_ = b.beat_ + (
    measure_count * b.barlength_ + (double)defmeasure_count
    ) * kDefaultMeasureLength;
  new_obj.measure_ = measure_idx;
  new_obj.barlength_ = barlength;
  barobjs_.push_back(new_obj);
}

/* @depreciated */
void TimingSegmentData::SeekByTime(double time)
{
  double beat = GetBeatFromTimeInLastSegment(time);
  double measure = GetMeasureFromBeatInLastSegment(beat);
  Seek(measure, beat, time);
}

void TimingSegmentData::SeekByMeasure(double measure)
{
  double beat = GetBeatFromMeasureInLastSegment(measure);
  double time = GetTimeFromBeatInLastSegment(beat);
  Seek(measure, beat, time);
}

void TimingSegmentData::Seek(double measure, double beat, double time)
{
  ASSERT(beat >= timingsegments_.back().beat_);
  ASSERT(time >= timingsegments_.back().time_);
  if (timingsegments_.back().beat_ == beat) return;

  BarObject &barobj = barobjs_.back();
  TimingSegment new_tobj(timingsegments_.back());
  new_tobj.clearForCopiedSegment();
  new_tobj.time_ = time;
  new_tobj.beat_ = beat;
  new_tobj.measure_ = measure;

  // if previous tempoobj did not manipulated, then overwrite to it.
  if (timingsegments_.back().is_manipulated_)
    timingsegments_.push_back(new_tobj);
  else
    timingsegments_.back() = new_tobj;
}

void TimingSegmentData::SetBPMChange(double bpm)
{
  if (timingsegments_.back().bpm_ == bpm) return;
  timingsegments_.back().bpm_ = bpm;
  timingsegments_.back().is_manipulated_ = true;
}


void TimingSegmentData::SetSTOP(double stop)
{
  timingsegments_.back().stoptime_ = stop;
  timingsegments_.back().is_manipulated_ = true;
}

void TimingSegmentData::SetDelay(double delay)
{
  timingsegments_.back().delaytime_ = delay;
  timingsegments_.back().is_manipulated_ = true;
}

void TimingSegmentData::SetWarp(double warp_length_in_beat)
{
  if (warp_length_in_beat < 0)
  {
    // Negative warp length : no assert (such case rarely happenes in few song data files)
    std::cerr << "Negative warp length found: " << warp_length_in_beat << " : automatically fixed to positive value.";
    warp_length_in_beat *= -1;
  }
  timingsegments_.back().warpbeat_ = warp_length_in_beat;
  timingsegments_.back().is_manipulated_ = true;
}

void TimingSegmentData::SetTick(uint32_t tick)
{
  timingsegments_.back().tick_ = tick;
  timingsegments_.back().is_manipulated_ = true;
}

void TimingSegmentData::SetScrollSpeedChange(double scrollspeed)
{
  if (scrollspeed <= 0)
  {
    std::cerr << "Scrollspeed less than or equal 0 is now allowed: " << scrollspeed << std::endl;
    scrollspeed = 1.0;
  }
  timingsegments_.back().scrollspeed_ = scrollspeed;
  timingsegments_.back().is_manipulated_ = true;
}

double TimingSegmentData::GetTimeFromBeatInLastSegment(double beat_delta) const
{
  return GetTimeFromBeatInTempoSegment(timingsegments_.back(), beat_delta);
}

double TimingSegmentData::GetBeatFromTimeInLastSegment(double time_delta_msec) const
{
  return GetBeatFromTimeInTempoSegment(timingsegments_.back(), time_delta_msec);
}

double TimingSegmentData::GetBeatFromMeasureInLastSegment(double m) const
{
  return GetBeatFromMeasureInBarSegment(barobjs_.back(), m, do_recover_measure_length_);
}

double TimingSegmentData::GetMeasureFromBeatInLastSegment(double beat) const
{
  auto &b = barobjs_.back();
  return b.measure_ + (beat - b.beat_) / b.barlength_ / kDefaultMeasureLength;
}

double TimingSegmentData::GetTimeFromMeasureInLastSegment(double m) const
{
  return GetTimeFromBeatInLastSegment(GetBeatFromMeasureInLastSegment(m));
}

double TimingSegmentData::GetMeasureFromTimeInLastSegment(double time) const
{
  return GetMeasureFromBeatInLastSegment(GetBeatFromTimeInLastSegment(time));
}

double TimingSegmentData::GetMaxBpm() const
{
  double maxbpm = timingsegments_.front().bpm_;
  for (auto& tobj: timingsegments_)
  {
    if (maxbpm < tobj.bpm_)
      maxbpm = tobj.bpm_;
  }
  return maxbpm;
}

double TimingSegmentData::GetMinBpm() const
{
  double minbpm = timingsegments_.front().bpm_;
  for (auto& tobj: timingsegments_)
  {
    if (minbpm > tobj.bpm_)
      minbpm = tobj.bpm_;
  }
  return minbpm;
}

bool TimingSegmentData::HasBpmChange() const
{
  double prev_bpm = timingsegments_.front().bpm_;
  for (auto& tobj: timingsegments_)
  {
    if (prev_bpm != tobj.bpm_)
      return true;
    prev_bpm = tobj.bpm_;
  }
  return false;
}

bool TimingSegmentData::HasStop() const
{
  for (auto& tobj: timingsegments_)
  {
    if (tobj.stoptime_ > 0)
      return true;
  }
  return false;
}

bool TimingSegmentData::HasWarp() const
{
  double prev = timingsegments_.front().warpbeat_;
  for (auto& tobj: timingsegments_)
  {
    if (prev != tobj.warpbeat_)
      return true;
    prev = tobj.warpbeat_;
  }
  return false;
}

void TimingSegmentData::SetMeasureLengthRecover(bool recover)
{
  // BMS need this option
  do_recover_measure_length_ = recover;
}

std::string TimingSegmentData::toString() const
{
  std::stringstream ss;
  ss << "BPM: " << timingsegments_.front().bpm_ << std::endl;
  ss << "MaxBPM: " << GetMaxBpm() << std::endl;
  ss << "MinBPM: " << GetMinBpm() << std::endl;
  ss << "BPMChange? : " << HasBpmChange() << std::endl;
  ss << "Warp? : " << HasWarp() << std::endl;
  ss << "Stop? : " << HasStop() << std::endl;
  if (use_detailed_info)
  {
    ss << "TSEGMENTS" << std::endl;
    for (auto &s : timingsegments_)
      ss << s.toString();
    ss << "BARS recovery " << (do_recover_measure_length_ ? "ON" : "OFF") << std::endl;
    for (auto &bar : barobjs_)
      ss << "B" << bar.beat_ << ",M" << bar.measure_ << " - length" << bar.barlength_ << std::endl;
  }
  return ss.str();
}

void TimingSegmentData::clear()
{
  do_recover_measure_length_ = false;
  timingsegments_.clear();
  barobjs_.clear();
  // Dummy object to avoid crashing
  timingsegments_.emplace_back(TimingSegment());
  timingsegments_.back().is_manipulated_ = true; /* prevent segment to be removed */
  barobjs_.emplace_back(BarObject());
}

void TimingSegmentData::swap(TimingSegmentData& timingdata)
{
  timingsegments_.swap(timingdata.timingsegments_);
  barobjs_.swap(timingdata.barobjs_);
}

const std::vector<BarObject>& TimingSegmentData::GetBarObjects() const
{
  return barobjs_;
}

double TimingSegmentData::GetBarLength(uint32_t measure) const
{
  BarObject b;
  b.measure_ = measure;
  auto it = std::lower_bound(barobjs_.rbegin(), std::prev(barobjs_.rend()), b);
  return (do_recover_measure_length_ && it->measure_ != measure)
    ? 1.0
    : it->barlength_;
}

void TimingSegmentData::UseDetailedInfo(bool v)
{
  use_detailed_info = v;
}

}
