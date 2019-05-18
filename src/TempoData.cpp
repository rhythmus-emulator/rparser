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

inline double GetBeatFromRowInTempoSegment(const TempoObject& n, double measure)
{
  return n.measure_length_changed_beat_
    + n.measure_length_ * (measure - n.measure_length_changed_idx_);
}

inline uint32_t GetMeasureFromBeatInTempoSegment(const TempoObject& n, double beat)
{
  return n.measure_length_changed_idx_ +
         static_cast<uint32_t>((beat - n.measure_length_changed_beat_) / n.measure_length_ + 0.000001 /* for calculation error */);
}

TempoObject::TempoObject()
: bpm_(kDefaultBpm), beat_(0), time_(0),
  measure_idx_(0), measure_length_changed_idx_(0), measure_length_changed_beat_(0), measure_length_(kDefaultMeasureLength),
  stoptime_(0), delaytime_(0), warpbeat_(0), scrollspeed_(1), tick_(1) {}

// called when TempoObject is copied for new block (e.g. Seek methods)
// As new object segment won't have same STOP/DELAY/WARP by default.
void TempoObject::clearForCopiedSegment()
{
  stoptime_ = 0;
  delaytime_ = 0;
  warpbeat_ = 0;
}

std::string TempoObject::toString()
{
  std::stringstream ss;
  ss << "BPM: " << bpm_ << std::endl;
  return ss.str();
}

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
  // set first bpm from metadata
  SetBPMChange(m.bpm);

  // extract tempo related object from chartdata
  static const NotePosTypes nbtypes[3] = { NotePosTypes::Beat, NotePosTypes::Time, NotePosTypes::Row };
  std::vector<const Note*> nobj_by_beat, nobj_by_tempo, nobj_by_row;
  std::vector<const Note*> *nobjvecs[] = { &nobj_by_beat, &nobj_by_tempo, &nobj_by_row };
  size_t nbidx[3] = { 0, 0, 0 };
  double nbarr[3];
  int cur_nobj_type_idx;
  const TempoNote* cur_nobj;
  float v;

  ConstSortedNoteObjects sorted;
  SortNoteObjectsByType(temponotedata_, sorted);
  nobj_by_beat.swap(sorted.nobj_by_beat);
  nobj_by_tempo.swap(sorted.nobj_by_tempo);
  nobj_by_row.swap(sorted.nobj_by_row);

  // make tempo segment from note objects
  while (nbidx[0] < nobj_by_beat.size() ||
         nbidx[1] < nobj_by_tempo.size() ||
         nbidx[2] < nobj_by_row.size())
  {
    // select tempo note which is in most early timing.
    if (nbidx[0] >= nobj_by_beat.size()) { nbarr[0] = std::numeric_limits<double>::max(); }
    else { nbarr[0] = nobj_by_beat[nbidx[0]]->pos().beat; }

    if (nbidx[1] >= nobj_by_tempo.size()) { nbarr[1] = std::numeric_limits<double>::max(); }
    else { nbarr[1] = GetBeatFromTimeInLastSegment(nobj_by_tempo[nbidx[1]]->pos().time_msec); }

    if (nbidx[2] >= nobj_by_row.size()) { nbarr[2] = std::numeric_limits<double>::max(); }
    else { nbarr[2] = GetBeatFromRowInLastSegment(nobj_by_row[nbidx[2]]->pos().measure); }

    cur_nobj_type_idx = GetSmallestIndex(nbarr, 3);
    cur_nobj = static_cast<const TempoNote*>( (*nobjvecs[cur_nobj_type_idx])[ nbidx[cur_nobj_type_idx] ] );
    nbidx[cur_nobj_type_idx]++;

    // seek for next tempo segment object and update note object beat value.
    // (exceptionally use const_cast for update beat position)
    SeekByBeat(nbarr[cur_nobj_type_idx]);
    const_cast<TempoNote*>(cur_nobj)->pos().beat = nbarr[cur_nobj_type_idx];

    // set tempo segment object attribute.
    switch (cur_nobj->subtype())
    {
    case NoteTempoTypes::kBpm:
      SetBPMChange(cur_nobj->GetFloatValue());
      break;
    case NoteTempoTypes::kBmsBpm:
      if (m.GetBPMChannel()->GetBpm(cur_nobj->GetIntValue(), v))
        SetBPMChange(v);
      else
        RPARSER_LOG("Failed to fetch BPM information.");
      break;
    case NoteTempoTypes::kStop:
      SetSTOP(cur_nobj->GetFloatValue());
      break;
    case NoteTempoTypes::kBmsStop:
      if (m.GetSTOPChannel()->GetStop(cur_nobj->GetIntValue(), v))
        SetSTOP(v);
      else
        RPARSER_LOG("Failed to fetch STOP information.");
      break;
    case NoteTempoTypes::kMeasure:
      SetMeasureLengthChange(cur_nobj->GetFloatValue());
      break;
    case NoteTempoTypes::kScroll:
      SetScrollSpeedChange(cur_nobj->GetFloatValue());
      break;
    case NoteTempoTypes::kTick:
      SetTick(cur_nobj->GetIntValue());
      break;
    case NoteTempoTypes::kWarp:
      SetWarp(cur_nobj->GetFloatValue());
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
  int min = 0, max = tempoobjs_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  uint32_t row_i = static_cast<uint32_t>(measure);
  while( l <= r )
  {
    int m = ( l + r ) / 2;

    if( ( m == min || tempoobjs_[m].measure_idx_ <= row_i) && ( m == max || row_i < tempoobjs_[m+1].measure_idx_ ) )
    {
      idx = m;
      break;
    }
    else if( tempoobjs_[m].measure_idx_ > row_i)
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }

  return GetBeatFromRowInTempoSegment(tempoobjs_[idx], measure);
}

double TempoData::GetMeasureFromBeat(double beat) const
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
  return tempoobjs_[idx].measure_idx_ + (beat - tempoobjs_[idx].beat_) / tempoobjs_[idx].measure_length_;
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

std::vector<double> TempoData::GetBeatFromRowArr(const std::vector<double>& sorted_row) const
{
  std::vector<double> r_beat;
  size_t idx = 0;
  for (double row: sorted_row)
  {
    // go to next segment if available.
    // -- measure pos might be same, so move as much as possible.
    while (idx+1 < tempoobjs_.size() && row > tempoobjs_[idx+1].measure_idx_) idx++;
    r_beat.push_back(GetBeatFromRowInTempoSegment(tempoobjs_[idx], row));
  }
  return r_beat;
}

void TempoData::SetFirstObjectFromMetaData(const MetaData &md)
{
  TempoObject &o = tempoobjs_.front();
  o.bpm_ = md.bpm;
}

void TempoData::SeekByTime(double time)
{
  ASSERT(time >= tempoobjs_.back().time_);
  if (time == tempoobjs_.back().time_) return;
  double beat = GetBeatFromTimeInLastSegment(time);
  TempoObject new_tobj(tempoobjs_.back());
  new_tobj.clearForCopiedSegment();
  new_tobj.time_ = time;
  new_tobj.beat_ = beat;
  new_tobj.measure_idx_ = GetMeasureFromBeatInTempoSegment(tempoobjs_.back(), beat);
  tempoobjs_.push_back(new_tobj);
}

void TempoData::SeekByBeat(double beat)
{
  ASSERT(beat >= tempoobjs_.back().beat_);
  if (beat == tempoobjs_.back().beat_) return;
  double time = GetTimeFromBeatInLastSegment(beat);
  TempoObject new_tobj(tempoobjs_.back());
  new_tobj.clearForCopiedSegment();
  new_tobj.time_ = time;
  new_tobj.beat_ = beat;
  new_tobj.measure_idx_ = GetMeasureFromBeatInTempoSegment(tempoobjs_.back(), beat);
  tempoobjs_.push_back(new_tobj);
}

void TempoData::SeekByRow(double row)
{
  // convert row into beat
  const TempoObject& n = tempoobjs_.back();
  SeekByBeat( GetBeatFromRowInTempoSegment(tempoobjs_.back(), row) );
}

void TempoData::SetBPMChange(double bpm)
{
  tempoobjs_.back().bpm_ = bpm;
}

void TempoData::SetMeasureLengthChange(double measure_length)
{
  TempoObject& tobj = tempoobjs_.back();
  // don't do unnecessary thing as much as possible
  if (tobj.measure_length_ == measure_length) return;
  // reinvalidate beat and measure index if necessary
  // and calculate measure of current tobj
  const uint32_t new_measure = GetMeasureFromBeatInTempoSegment(tobj, tobj.beat_);
  const double new_measure_starting_beat = tobj.measure_length_changed_beat_ +
                                           (new_measure - tobj.measure_length_changed_beat_) * tobj.measure_length_;
  if (tobj.measure_length_changed_beat_ != new_measure_starting_beat)
  {
    tobj.measure_length_changed_idx_ = new_measure;
    tobj.measure_length_changed_beat_ = new_measure_starting_beat;
  }
  tobj.measure_length_ = measure_length;
}

void TempoData::SetSTOP(double stop)
{
  tempoobjs_.back().stoptime_ = stop;
}

void TempoData::SetDelay(double delay)
{
  tempoobjs_.back().delaytime_ = delay;
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
}

void TempoData::SetTick(uint32_t tick)
{
  tempoobjs_.back().tick_ = tick;
}

void TempoData::SetScrollSpeedChange(double scrollspeed)
{
  if (scrollspeed <= 0)
  {
    std::cerr << "Scrollspeed less than or equal 0 is now allowed: " << scrollspeed << std::endl;
    scrollspeed = 1.0;
  }
  tempoobjs_.back().scrollspeed_ = scrollspeed;
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
  return GetBeatFromRowInTempoSegment(tempoobjs_.back(), row);
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
  tempoobjs_.clear();
  // Dummy object to avoid crashing
  tempoobjs_.emplace_back(TempoObject());
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
