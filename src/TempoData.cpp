#include "TempoData.h"
#include "MetaData.h"
#include "ChartUtil.h"
#include <math.h>

namespace rparser
{

inline double GetTimeFromBeatInTempoSegment(const TempoObject& tobj, double beat)
{
  const double msec_per_beat = 60.0 * 1000 / tobj.bpm_ / tobj.scrollspeed_;
  const double beat_delta = beat - tobj.beat_ - tobj.warpbeat_;
  if (beat_delta < 0) return tobj.time_ + tobj.stoptime_;
  else return tobj.time_ + tobj.stoptime_ + tobj.delaytime_ + beat_delta * msec_per_beat;
}

inline double GetBeatFromTimeInTempoSegment(const TempoObject& tobj, double time_msec)
{
  const double beat_per_msec = tobj.bpm_ * tobj.scrollspeed_ / 60 / 1000;
  const double time_delta = time_msec - tobj.time_ - (tobj.stoptime_ + tobj.delaytime_);
  if (time_delta <= 0) return tobj.beat_;
  return tobj.beat_ + tobj.warpbeat_ + time_delta * beat_per_msec;
}

inline double GetBeatFromRowInBarSegment(const BarObject& b, double measure)
{
  return b.beatpos_ + (measure - b.baridx_) * b.barlength_;
}

inline uint32_t GetMeasureFromBeatInBarSegment(const BarObject& b, double beat)
{
  return b.baridx_ + static_cast<uint32_t>((beat - b.beatpos_) / b.barlength_ + 0.000001 /* for calculation error */);
}

TempoObject::TempoObject()
: bpm_(kDefaultBpm), beat_(0), time_(0),
  stoptime_(0), delaytime_(0), warpbeat_(0), scrollspeed_(1), tick_(1), is_manipulated_(false) {}

// called when TempoObject is copied for new block (e.g. Seek methods)
// As new object segment won't have same STOP/DELAY/WARP by default.
void TempoObject::clearForCopiedSegment()
{
  stoptime_ = 0;
  delaytime_ = 0;
  warpbeat_ = 0;
  is_manipulated_ = false;
}

std::string TempoObject::toString()
{
  std::stringstream ss;
  ss << "BPM: " << bpm_ << std::endl;
  return ss.str();
}

BarObject::BarObject()
  : beatpos_(0), barlength_(4.0), baridx_(0), is_implicit_(false) {}

BarObject::BarObject(double beatpos, double barlength, uint32_t baridx, bool is_implicit)
  : beatpos_(beatpos), barlength_(barlength), baridx_(baridx), is_implicit_(is_implicit) {}

TempoData::TempoData()
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

void TempoData::Invalidate(const MetaData& m)
{
  float v;

  // Set first bpm from metadata
  SetBPMChange(m.bpm);

  // Set measure info first.
  // - by calculation of measure --> beat pos, we can set objects by bar(measure) pos.
  // - if measure information with time position required, we need to make another invalidation method
  //   which processes BPM(Tempo segment) first, then fill measure segment later,
  //   ... which means bar pos is inavailable in this case.
  std::vector<const TempoNote*> tobj_measures_bar, tobj_measures_beat;
  for (const TempoNote& n : temponotedata_)
  {
    if (n.postype() == NotePosTypes::Time)
    {
      std::cerr << "[Warning] Time position ignored (unsupported)." << std::endl;
      continue;
    }
    if (n.subtype() == NoteTempoTypes::kMeasure)
    {
      if (n.postype() == NotePosTypes::Beat) tobj_measures_beat.push_back(&n);
      else if (n.postype() == NotePosTypes::Bar) tobj_measures_bar.push_back(&n);
    }
  }
  std::sort(tobj_measures_beat.begin(), tobj_measures_beat.end(), [](const Note* lhs, const Note* rhs)
  { return lhs->pos().beat < rhs->pos().beat; });
  std::sort(tobj_measures_bar.begin(), tobj_measures_bar.end(), [](const TempoNote* lhs, const TempoNote* rhs)
  { return lhs->pos().measure < rhs->pos().measure; });
  for (const TempoNote* n : tobj_measures_beat) SetMeasureLengthChange(n->beat, n->GetFloatValue());
  for (const TempoNote* n : tobj_measures_bar) SetMeasureLengthChange((uint32_t)n->measure, n->GetFloatValue());

  // Calculate bar --> beat if necessary.
#if 0
  std::sort(temponotedata_.begin(), temponotedata_.end(), [](const Note* lhs, const Note* rhs)
  { return lhs->pos().measure < rhs->pos().measure; });
  std::vector<double> vBarpos, vBeatpos;
  size_t vNidx = 0;
  for (TempoNote& n : temponotedata_)
    if (n.postype() == NotePosTypes::Bar)
      vBarpos.push_back(n.measure);
  vBeatpos = std::move(GetBeatFromBarArr(vBarpos));
  for (TempoNote& n : temponotedata_)
    if (n.postype() == NotePosTypes::Bar)
      n.beat = vBeatpos[vNidx++];
#endif
  for (TempoNote& n : temponotedata_)
    if (n.postype() == NotePosTypes::Bar)
      n.beat = GetBeatFromRow(n.measure);

  // Sort total objects with beat position
  /**
   * COMMENT
   * Bpm channel and Bmsbpm channel object may exist in same place,
   * and it originally produces unknown result, and as it's edge case we can ignore it.
   * But L99^ song total time is much different unless latter object(Bmsbpm) has bpm precedence.
   * So, in shortcut, we do sorting in subtype to make Bmsbpm object has more precedence.
   */
  std::sort(temponotedata_.begin(), temponotedata_.end(), [](const TempoNote& lhs, const TempoNote& rhs)
  { return lhs.pos().beat == rhs.pos().beat ? lhs.subtype() < rhs.subtype() : lhs.pos().beat < rhs.pos().beat; });

  // Make tempo segments
  for (const TempoNote& n : temponotedata_)
  {
    if (n.postype() == NotePosTypes::Time) continue;
    if (n.subtype() == NoteTempoTypes::kMeasure) continue;

    // seek for next tempo segment object and update note object beat value.
    SeekByBeat(n.beat);
    const_cast<TempoNote&>(n).time_msec = tempoobjs_.back().time_;

    // set tempo segment object attribute.
    switch (n.subtype())
    {
    case NoteTempoTypes::kBpm:
      SetBPMChange(n.GetFloatValue());
      break;
    case NoteTempoTypes::kBmsBpm:
      if (m.GetBPMChannel()->GetBpm(n.GetIntValue(), v))
        SetBPMChange(v);
      else
        RPARSER_LOG("Failed to fetch BPM information.");
      break;
    case NoteTempoTypes::kStop:
      SetSTOP(n.GetFloatValue());
      break;
    case NoteTempoTypes::kBmsStop:
      // STOP value of 192 means 1 measure stop (4 beat).
      if (m.GetSTOPChannel()->GetStop(n.GetIntValue(), v))
        SetSTOP(v / 192.0 * 4.0);
      else
        RPARSER_LOG("Failed to fetch STOP information.");
      break;
    case NoteTempoTypes::kMeasure:
      /** Do nothing */
      break;
    case NoteTempoTypes::kScroll:
      SetScrollSpeedChange(n.GetFloatValue());
      break;
    case NoteTempoTypes::kTick:
      SetTick(n.GetIntValue());
      break;
    case NoteTempoTypes::kWarp:
      SetWarp(n.GetFloatValue());
      break;
    }
  }

  // sort internal temponotedata internally by beat.
  //std::sort(temponotedata_.begin(), temponotedata_.end());
}

double TempoData::GetTimeFromBeat(double beat) const
{
  // binary search to find proper tempoobject segment.
  int min = 0, max = tempoobjs_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  while( l <= r )
  {
    int m = ( l + r ) / 2;
    if( ( m == min || tempoobjs_[m].beat_ <= beat ) && ( m == max || beat < tempoobjs_[m+1].beat_ ) )
    {
      idx = m;
      break;
    }
    else if( tempoobjs_[m].beat_ > beat )
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }
  return GetTimeFromBeatInTempoSegment(tempoobjs_[idx], beat);
}

double TempoData::GetBeatFromTime(double time) const
{
  // binary search to find proper tempoobject segment.
  int min = 0, max = tempoobjs_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  while( l <= r )
  {
    int m = ( l + r ) / 2;
    if( ( m == min || tempoobjs_[m].time_ <= time ) && ( m == max || time < tempoobjs_[m+1].time_ ) )
    {
      idx = m;
      break;
    }
    else if( tempoobjs_[m].time_ > time )
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }
  return GetBeatFromTimeInTempoSegment(tempoobjs_[idx], time);
}

double TempoData::GetBeatFromRow(double measure) const
{
  /**
   * NOTE: CANNOT be replaced with measure + num / deno method
   * as we should care about measure length.
   */

  // binary search to find proper tempoobject segment.
  int min = 0, max = barobjs_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  uint32_t row_i = static_cast<uint32_t>(floorl(measure));
  while( l <= r )
  {
    int m = ( l + r ) / 2;

    if( ( m == min || barobjs_[m].baridx_ <= row_i) && ( m == max || row_i < barobjs_[m+1].baridx_) )
    {
      idx = m;
      break;
    }
    else if(barobjs_[m].baridx_ > row_i)
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }

  return GetBeatFromRowInBarSegment(barobjs_[idx], measure);
}

double TempoData::GetMeasureFromBeat(double beat) const
{
  // binary search to find proper tempoobject segment.
  int min = 0, max = barobjs_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  while( l <= r )
  {
    int m = ( l + r ) / 2;
    if( ( m == min || barobjs_[m].beatpos_ <= beat ) && ( m == max || beat < barobjs_[m+1].beatpos_) )
    {
      idx = m;
      break;
    }
    else if(barobjs_[m].beatpos_ > beat )
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }
  return barobjs_[idx].baridx_ + (beat - barobjs_[idx].beatpos_) / barobjs_[idx].barlength_;
}

std::vector<double> TempoData::GetTimeFromBeatArr(const std::vector<double>& sorted_beat) const
{
  std::vector<double> r_time;
  size_t idx = 0;
  for (double beat: sorted_beat)
  {
    // go to next segment if available.
    if (idx+1 < tempoobjs_.size() && beat > tempoobjs_[idx+1].beat_) idx++;
    r_time.push_back(GetTimeFromBeatInTempoSegment(tempoobjs_[idx], beat));
  }
  return r_time;
}

std::vector<double> TempoData::GetBeatFromTimeArr(const std::vector<double>& sorted_time) const
{
  std::vector<double> r_beat;
  size_t idx = 0;
  for (double time: sorted_time)
  {
    // go to next segment if available.
    if (idx+1 < tempoobjs_.size() && time > tempoobjs_[idx+1].time_) idx++;
    r_beat.push_back(GetBeatFromTimeInTempoSegment(tempoobjs_[idx], time));
  }
  return r_beat;
}

std::vector<double> TempoData::GetBeatFromBarArr(const std::vector<double>& sorted_row) const
{
  std::vector<double> r_beat;
  size_t idx = 0;
  for (double row: sorted_row)
  {
    // go to next segment if available.
    // -- measure pos might be same, so move as much as possible.
    while (idx+1 < barobjs_.size() && row > barobjs_[idx+1].baridx_) idx++;
    r_beat.push_back(GetBeatFromRowInBarSegment(barobjs_[idx], row));
  }
  return r_beat;
}

void TempoData::SetFirstObjectFromMetaData(const MetaData &md)
{
  TempoObject &o = tempoobjs_.front();
  o.bpm_ = md.bpm;
}

void TempoData::SetMeasureLengthChange(uint32_t measure_idx, double barlength)
{
  // don't do unnecessary thing as much as possible
  if (barlength <= 0)
  {
    std::cerr << "[Warning] measure length below zero ignored." << std::endl;
    return;
  }
  // Only use last segment.
  BarObject& b = barobjs_.back();
  ASSERT(measure_idx >= b.baridx_);
  if (!b.is_implicit_ && measure_idx == b.baridx_) return; /* may be caused by do_recover_measure_length */
  if (b.barlength_ == barlength) return;

  // Set bar segment
  BarObject new_obj(b);
  new_obj.beatpos_ = b.beatpos_ + (measure_idx - b.baridx_) * b.barlength_;
  new_obj.baridx_ = measure_idx;
  new_obj.barlength_ = barlength;
  new_obj.is_implicit_ = false;

  // If same measure_idx, then update previous one.
  if (measure_idx == b.baridx_)
    barobjs_.back() = new_obj;
  else
    barobjs_.push_back(new_obj);

  // If need to recover measure length (BMS type)
  if (do_recover_measure_length_)
    barobjs_.emplace_back(BarObject(new_obj.beatpos_ + barlength, kDefaultMeasureLength, new_obj.baridx_+1, true ));
}

void TempoData::SetMeasureLengthChange(double beat_pos, double measure_length)
{
  // Only use last segment.
  const BarObject& b = barobjs_.back();
  ASSERT(beat_pos >= b.beatpos_);
  if (!b.is_implicit_ && beat_pos == b.beatpos_) return; /* may be caused by do_recover_measure_length */
  SetMeasureLengthChange(b.baridx_ + static_cast<uint32_t>((beat_pos - b.beatpos_) / b.barlength_), measure_length);
}

void TempoData::SeekByTime(double time)
{
  double beat = GetBeatFromTimeInLastSegment(time);
  Seek(beat, time);
}

void TempoData::SeekByBeat(double beat)
{
  double time = GetTimeFromBeatInLastSegment(beat);
  Seek(beat, time);
}

void TempoData::SeekByBar(double bar)
{
  SeekByBeat( GetBeatFromRow(bar) );
}

void TempoData::Seek(double beat, double time)
{
  ASSERT(beat >= tempoobjs_.back().beat_);
  ASSERT(time >= tempoobjs_.back().time_);
  if (tempoobjs_.back().beat_ == beat) return;

  TempoObject new_tobj(tempoobjs_.back());
  new_tobj.clearForCopiedSegment();
  new_tobj.time_ = time;
  new_tobj.beat_ = beat;

  // if previous tempoobj did not manipulated, then overwrite to it.
  if (tempoobjs_.back().is_manipulated_)
    tempoobjs_.push_back(new_tobj);
  else
    tempoobjs_.back() = new_tobj;
}

void TempoData::SetBPMChange(double bpm)
{
  if (tempoobjs_.back().bpm_ == bpm) return;
  tempoobjs_.back().bpm_ = bpm;
  tempoobjs_.back().is_manipulated_ = true;
}


void TempoData::SetSTOP(double stop)
{
  tempoobjs_.back().stoptime_ = stop;
  tempoobjs_.back().is_manipulated_ = true;
}

void TempoData::SetDelay(double delay)
{
  tempoobjs_.back().delaytime_ = delay;
  tempoobjs_.back().is_manipulated_ = true;
}

void TempoData::SetWarp(double warp_length_in_beat)
{
  if (warp_length_in_beat < 0)
  {
    // Negative warp length : no assert (such case rarely happenes in few song data files)
    std::cerr << "Negative warp length found: " << warp_length_in_beat << " : automatically fixed to positive value.";
    warp_length_in_beat *= -1;
  }
  tempoobjs_.back().warpbeat_ = warp_length_in_beat;
  tempoobjs_.back().is_manipulated_ = true;
}

void TempoData::SetTick(uint32_t tick)
{
  tempoobjs_.back().tick_ = tick;
  tempoobjs_.back().is_manipulated_ = true;
}

void TempoData::SetScrollSpeedChange(double scrollspeed)
{
  if (scrollspeed <= 0)
  {
    std::cerr << "Scrollspeed less than or equal 0 is now allowed: " << scrollspeed << std::endl;
    scrollspeed = 1.0;
  }
  tempoobjs_.back().scrollspeed_ = scrollspeed;
  tempoobjs_.back().is_manipulated_ = true;
}

double TempoData::GetTimeFromBeatInLastSegment(double beat_delta) const
{
  return GetTimeFromBeatInTempoSegment(tempoobjs_.back(), beat_delta);
}

double TempoData::GetBeatFromTimeInLastSegment(double time_delta_msec) const
{
  return GetBeatFromTimeInTempoSegment(tempoobjs_.back(), time_delta_msec);
}

double TempoData::GetBeatFromRowInLastSegment(double row) const
{
  return GetBeatFromRowInBarSegment(barobjs_.back(), row);
}

double TempoData::GetMaxBpm() const
{
  double maxbpm = tempoobjs_.front().bpm_;
  for (auto& tobj: tempoobjs_)
  {
    if (maxbpm < tobj.bpm_)
      maxbpm = tobj.bpm_;
  }
  return maxbpm;
}

double TempoData::GetMinBpm() const
{
  double minbpm = tempoobjs_.front().bpm_;
  for (auto& tobj: tempoobjs_)
  {
    if (minbpm > tobj.bpm_)
      minbpm = tobj.bpm_;
  }
  return minbpm;
}

bool TempoData::HasBpmChange() const
{
  double prev_bpm = tempoobjs_.front().bpm_;
  for (auto& tobj: tempoobjs_)
  {
    if (prev_bpm != tobj.bpm_)
      return true;
    prev_bpm = tobj.bpm_;
  }
  return false;
}

bool TempoData::HasStop() const
{
  for (auto& tobj: tempoobjs_)
  {
    if (tobj.stoptime_ > 0)
      return true;
  }
  return false;
}

bool TempoData::HasWarp() const
{
  double prev = tempoobjs_.front().warpbeat_;
  for (auto& tobj: tempoobjs_)
  {
    if (prev != tobj.warpbeat_)
      return true;
    prev = tobj.warpbeat_;
  }
  return false;
}

void TempoData::SetMeasureLengthRecover(bool recover)
{
  // BMS need this option
  do_recover_measure_length_ = recover;
}

std::string TempoData::toString() const
{
  std::stringstream ss;
  ss << "BPM: " << tempoobjs_.front().bpm_ << std::endl;
  ss << "MaxBPM: " << GetMaxBpm() << std::endl;
  ss << "MinBPM: " << GetMinBpm() << std::endl;
  ss << "BPMChange? : " << HasBpmChange() << std::endl;
  ss << "Warp? : " << HasWarp() << std::endl;
  ss << "Stop? : " << HasStop() << std::endl;
  return ss.str();
}

void TempoData::clear()
{
  do_recover_measure_length_ = false;
  tempoobjs_.clear();
  barobjs_.clear();
  // Dummy object to avoid crashing
  tempoobjs_.emplace_back(TempoObject());
  barobjs_.emplace_back(BarObject());
}

void TempoData::swap(TempoData& tempodata)
{
  std::swap(tempoobjs_, tempodata.tempoobjs_);
}

NoteData<TempoNote>& TempoData::GetTempoNoteData()
{
  return temponotedata_;
}

}
