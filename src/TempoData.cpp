#include "TempoData.h"
#include "MetaData.h"
#include "ChartUtil.h"
#include <math.h>

namespace rparser
{

inline double GetTimeFromBarInTempoSegment(const TimingSegment& tobj, double barpos)
{
  const double msec_per_beat = 60.0 * 1000 / tobj.bpm_ / tobj.scrollspeed_;
  const double beat_delta = barpos - tobj.barpos_ - tobj.warpbeat_ /* XXX: warpbeat is not affected by barlength? */;
  if (beat_delta < 0) return tobj.time_ + tobj.stoptime_;
  else return tobj.time_ + tobj.stoptime_ + tobj.delaytime_ + beat_delta * msec_per_beat;
}

inline double GetBarFromTimeInTempoSegment(const TimingSegment& tobj, double time_msec)
{
  const double beat_per_msec = tobj.bpm_ * tobj.scrollspeed_ / 60 / 1000;
  const double time_delta = time_msec - tobj.time_ - (tobj.stoptime_ + tobj.delaytime_);
  if (time_delta <= 0) return tobj.barpos_;
  return tobj.barpos_ + tobj.warpbeat_ + time_delta * beat_per_msec;
}

inline double GetBarFromBeatInBarSegment(const BarObject& b, double beat, bool recover_length)
{
  double diff = beat - (double)b.beat_;
  if (recover_length && diff > 1.0)
    return b.barpos_ + (diff - 1.0) * kDefaultMeasureLength + b.barlength_;
  else
    return b.barpos_ + diff * b.barlength_;
}

inline double GetBeatFromBarInBarSegment(const BarObject& b, double bar, bool recover_length)
{
  double diff = bar - b.barpos_;
  if (recover_length && diff > b.barlength_)
    return b.beat_ + 1.0 + (diff - b.barlength_) / kDefaultMeasureLength;
  else
    return b.beat_ + diff / b.barlength_;
}

inline uint32_t GetMeasureFromBarInBarSegment(const BarObject& b, double bar, bool recover_length)
{
  return static_cast<uint32_t>(
    GetBeatFromBarInBarSegment(b, bar, recover_length)
    + 0.000001 /* for calculation error */
    );
}

TimingSegment::TimingSegment()
: bpm_(kDefaultBpm), beat_(0), time_(0),
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

std::string TimingSegment::toString()
{
  std::stringstream ss;
  ss << "BPM: " << bpm_ << std::endl;
  return ss.str();
}

BarObject::BarObject()
  : barpos_(0), barlength_(4.0), beat_(0) {}

BarObject::BarObject(double barpos, double barlength, uint32_t measure)
  : barpos_(barpos), barlength_(barlength), beat_(measure) {}

bool BarObject::operator<(const BarObject &other) const
{
  return beat_ < other.beat_;
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

void TimingSegmentData::Invalidate(const MetaData& m)
{
  float v;

  // Set first bpm from metadata
  SetBPMChange(m.bpm);

  // Set measure info first, as measure length effects to time calculation.
  for (auto *obj : timingdata_.get_track(TimingObjectTypes::kMeasure))
  {
    auto &tobj = *static_cast<TimingObject*>(obj);
    SetMeasureLengthChange(tobj.beat, tobj.GetFloatValue());
  }

  /**
   * COMMENT
   * Bpm channel and Bmsbpm channel object may exist in same place,
   * and it originally produces unknown result, and as it's edge case we can ignore it.
   * But L99^ song total time is much different unless latter object(Bmsbpm) has bpm precedence.
   * So, in shortcut, we do sorting in subtype to make Bmsbpm object has more precedence.
   * -- all_track_iterator automatically do this, so no need to care about it here.
   */

  auto timingobjiter = timingdata_.GetAllTrackIterator();

  // Make tempo segments
  while (!timingobjiter.is_end())
  {
    auto &tobj = static_cast<TimingObject&>(*timingobjiter);
    if (tobj.get_track() == TimingObjectTypes::kMeasure) continue;

    // seek for next tempo segment object and update note object beat value.
    SeekByBeat(tobj.beat);
    tobj.SetTime(timingsegments_.back().time_);

    // set tempo segment object attribute.
    switch (tobj.get_track())
    {
    case TimingObjectTypes::kMeasure:
      SetMeasureLengthChange(tobj.beat, tobj.GetFloatValue());
      break;
    case TimingObjectTypes::kScroll:
      SetScrollSpeedChange(tobj.GetFloatValue());
      break;
    case TimingObjectTypes::kBpm:
      SetBPMChange(tobj.GetFloatValue());
      break;
    case TimingObjectTypes::kBmsBpm:
      if (m.GetBPMChannel()->GetBpm(tobj.GetIntValue(), v))
        SetBPMChange(v);
      else
        RPARSER_LOG("Failed to fetch BPM information.");
      break;
    case TimingObjectTypes::kStop:
      SetSTOP(tobj.GetFloatValue());
      break;
    case TimingObjectTypes::kBmsStop:
      // STOP value of 192 means 1 measure stop (4 beat).
      if (m.GetSTOPChannel()->GetStop(tobj.GetIntValue(), v))
        SetSTOP(v / 192.0 * 4.0);
      else
        RPARSER_LOG("Failed to fetch STOP information.");
      break;
    case TimingObjectTypes::kTick:
      SetTick(tobj.GetIntValue());
      break;
    case TimingObjectTypes::kWarp:
      SetWarp(tobj.GetFloatValue());
      break;
    }

    ++timingobjiter;
  }
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
  return GetTimeFromBarInTempoSegment(timingsegments_[idx], GetBarFromBeat(beat));
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
  return GetBeatFromBar(GetBarFromTimeInTempoSegment(timingsegments_[idx], time));
}

double TimingSegmentData::GetBarFromBeat(double beat) const
{
  /**
   * NOTE: CANNOT be replaced with measure + num / deno method
   * as we should care about measure length.
   */

  // binary search to find proper tempoobject segment.
  int min = 0, max = barobjs_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  uint32_t measure = static_cast<uint32_t>(floorl(beat));
  while( l <= r )
  {
    int m = ( l + r ) / 2;

    if( ( m == min || barobjs_[m].beat_ <= measure) && ( m == max || measure < barobjs_[m+1].beat_) )
    {
      idx = m;
      break;
    }
    else if(barobjs_[m].beat_ > measure)
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }

  return GetBarFromBeatInBarSegment(barobjs_[idx], beat, do_recover_measure_length_);
}

double TimingSegmentData::GetBeatFromBar(double bar) const
{
  // binary search to find proper tempoobject segment.
  int min = 0, max = barobjs_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  while( l <= r )
  {
    int m = ( l + r ) / 2;
    if( ( m == min || barobjs_[m].barpos_ <= bar ) && ( m == max || bar < barobjs_[m+1].barpos_) )
    {
      idx = m;
      break;
    }
    else if(barobjs_[m].barpos_ > bar )
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }
  return GetBeatFromBarInBarSegment(barobjs_[idx], bar, do_recover_measure_length_);
}

std::vector<double> TimingSegmentData::GetTimeFromBeatArr(const std::vector<double>& sorted_beat) const
{
  std::vector<double> r_bar, r_time;
  size_t idx = 0;
  for (double beat : sorted_beat)
  {
    if (idx + 1 < barobjs_.size() && beat > barobjs_[idx + 1].beat_) idx++;
    r_bar.push_back(GetBarFromBeatInBarSegment(barobjs_[idx], beat, do_recover_measure_length_));
  }

  idx = 0;
  for (double bar: r_bar)
  {
    // go to next segment if available.
    if (idx+1 < timingsegments_.size() && bar > timingsegments_[idx+1].barpos_) idx++;
    r_time.push_back(GetTimeFromBarInTempoSegment(timingsegments_[idx], bar));
  }
  return r_time;
}

std::vector<double> TimingSegmentData::GetBeatFromTimeArr(const std::vector<double>& sorted_time) const
{
  std::vector<double> r_bar, r_beat;
  size_t idx = 0;
  for (double time: sorted_time)
  {
    // go to next segment if available.
    if (idx+1 < timingsegments_.size() && time > timingsegments_[idx+1].time_) idx++;
    r_bar.push_back(GetBarFromTimeInTempoSegment(timingsegments_[idx], time));
  }

  idx = 0;
  for (double bar : r_bar)
  {
    if (idx + 1 < barobjs_.size() && bar > barobjs_[idx + 1].barpos_) idx++;
    r_beat.push_back(GetBeatFromBarInBarSegment(barobjs_[idx], bar, do_recover_measure_length_));
  }
  return r_beat;
}

std::vector<double> TimingSegmentData::GetBarFromBeatArr(const std::vector<double>& sorted_beat) const
{
  std::vector<double> r_beat;
  size_t idx = 0;
  for (double beat: sorted_beat)
  {
    // go to next segment if available.
    // -- measure pos might be same, so move as much as possible.
    while (idx+1 < barobjs_.size() && beat > (double)barobjs_[idx+1].beat_) idx++;
    r_beat.push_back(GetBarFromBeatInBarSegment(barobjs_[idx], beat, do_recover_measure_length_));
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
  ASSERT(measure_idx >= b.beat_); /* same bar editing with do_recover_measure_length */
  if (b.barlength_ == barlength) return;

  // If same measure_idx, then update previous one.
  if (measure_idx == b.beat_)
  {
    barobjs_.back().barlength_ = barlength;
    return;
  }

  // Set bar segment
  BarObject new_obj(b);
  uint32_t measure_count = measure_idx - b.beat_;
  uint32_t defmeasure_count = 0;
  if (do_recover_measure_length_)
  {
    defmeasure_count = measure_count - 1;
    measure_count = 1;
  }
  new_obj.barpos_ = b.barpos_ + measure_count * b.barlength_ + defmeasure_count * kDefaultMeasureLength;
  new_obj.beat_ = measure_idx;
  new_obj.barlength_ = barlength;
  barobjs_.push_back(new_obj);
}

/* @depreciated */
void TimingSegmentData::SeekByTime(double time)
{
  double beat = GetBarFromTimeInLastSegment(time);
  Seek(beat, time);
}

void TimingSegmentData::SeekByBeat(double beat)
{
  double time = GetTimeFromBarInLastSegment(beat);
  Seek(beat, time);
}

void TimingSegmentData::Seek(double beat, double time)
{
  ASSERT(beat >= timingsegments_.back().beat_);
  ASSERT(time >= timingsegments_.back().time_);
  if (timingsegments_.back().beat_ == beat) return;

  BarObject &barobj = barobjs_.back();
  TimingSegment new_tobj(timingsegments_.back());
  new_tobj.clearForCopiedSegment();
  new_tobj.time_ = time;
  new_tobj.beat_ = beat;
  new_tobj.barpos_ = barobj.barpos_ + (do_recover_measure_length_ && (beat - barobj.beat_) > 1)
    ? (beat - barobj.beat_) * barobj.barlength_
    : barobj.barlength_ + (beat - barobj.beat_ - 1) * kDefaultMeasureLength
    ;

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

double TimingSegmentData::GetTimeFromBarInLastSegment(double beat_delta) const
{
  return GetTimeFromBarInTempoSegment(timingsegments_.back(), beat_delta);
}

double TimingSegmentData::GetBarFromTimeInLastSegment(double time_delta_msec) const
{
  return GetBarFromTimeInTempoSegment(timingsegments_.back(), time_delta_msec);
}

double TimingSegmentData::GetBarFromBeatInLastSegment(double beat) const
{
  return GetBarFromBeatInBarSegment(barobjs_.back(), beat, do_recover_measure_length_);
}

double TimingSegmentData::GetBeatFromBarInLastSegment(double bar) const
{
  auto &b = barobjs_.back();
  return b.beat_ + (bar - b.barpos_) / b.barlength_;
}

double TimingSegmentData::GetTimeFromBeatInLastSegment(double beat) const
{
  return GetTimeFromBarInLastSegment(GetBarFromBeatInLastSegment(beat));
}

double TimingSegmentData::GetBeatFromTimeInLastSegment(double time) const
{
  return GetBeatFromBarInLastSegment(GetBarFromTimeInLastSegment(time));
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
  return ss.str();
}

void TimingSegmentData::clear()
{
  do_recover_measure_length_ = false;
  timingsegments_.clear();
  barobjs_.clear();
  // Dummy object to avoid crashing
  timingsegments_.emplace_back(TimingSegment());
  barobjs_.emplace_back(BarObject());
}

void TimingSegmentData::swap(TimingSegmentData& timingdata)
{
  timingdata_.swap(timingdata.timingdata_);
  timingsegments_.swap(timingdata.timingsegments_);
  barobjs_.swap(timingdata.barobjs_);
}

TimingData& TimingSegmentData::GetTimingData()
{
  return timingdata_;
}

const TimingData& TimingSegmentData::GetTimingData() const
{
  return const_cast<TimingSegmentData*>(this)->GetTimingData();
}

const std::vector<BarObject>& TimingSegmentData::GetBarObjects() const
{
  return barobjs_;
}

double TimingSegmentData::GetBarLength(uint32_t measure) const
{
  BarObject b;
  b.beat_ = measure;
  auto it = std::lower_bound(barobjs_.rbegin(), barobjs_.rend(), b);
  return (do_recover_measure_length_ && it->beat_ != measure)
    ? kDefaultMeasureLength
    : it->barlength_;
}

}
