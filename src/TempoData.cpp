#include "TempoData.h"
#include "Chart.h"
#include "ChartUtil.h"

namespace rparser
{

inline double GetDeltaTimeFromBeatInTempoSegment(const TempoObject& tobj, double beat_delta)
{
  const double msec_per_beat = 60.0 * 1000 / tobj.bpm_;
  return beat_delta * msec_per_beat;
}

inline double GetDeltaBeatFromTimeInTempoSegment(const TempoObject& tobj, double time_delta_msec)
{
  const double beat_per_msec = tobj.bpm_ / 60 / 1000;
  return time_delta_msec * beat_per_msec;
}

inline double GetBeatFromRowInTempoSegment(const TempoObject& n, const Row& row)
{
  return n.measure_length_changed_beat_
    + n.measure_length_ * (row.measure - n.measure_idx_ + row.num / (double)row.deno);
}

TempoObject::TempoObject()
: bpm_(kDefaultBpm), beat_(0), time_(0),
  measure_length_changed_beat_(0), measure_length_(4), measure_idx_(0),
  stoptime_(0), delaytime_(0), warpbeat_(0), scrollspeed_(1), tick_(1) {}

TempoObject::TempoObject(const TempoObject& o)
: bpm_(o.bpm_), beat_(o.beat_), time_(o.time_),
  measure_length_changed_beat_(o.measure_length_changed_beat_),
  measure_length_(o.measure_length_), measure_idx_(0),
  stoptime_(0), delaytime_(0), warpbeat_(0),  // cleared when copied
  scrollspeed_(o.scrollspeed_), tick_(o.tick_) {}

std::string TempoObject::toString()
{
  std::stringstream ss;
  ss << "BPM: " << bpm_ << std::endl;
  return ss.str();
}

TempoData::TempoData()
{
  tempoobjs_.push_back(TempoObject());
}

TempoData::TempoData(const Chart& c)
{
  // extract tempo related object from chartdata
  std::vector<const Note*> nobj_by_beat, nobj_by_tempo, nobj_by_row;
  int nbidx=0, ntidx=0;
  NotePosTypes cur_nobj_type;
  const Note* cur_nobj;
  float v;

  ConstSortedNoteObjects sorted;
  SortNoteObjectsByType(c.GetNoteData(), sorted);
  nobj_by_beat.swap(sorted.nobj_by_beat);
  nobj_by_tempo.swap(sorted.nobj_by_tempo);
  nobj_by_row.swap(sorted.nobj_by_row);

  // make tempo segment from note objects
  while (nbidx < nobj_by_beat.size() && ntidx < nobj_by_tempo.size())
  {
    if (nbidx >= nobj_by_beat.size()) cur_nobj_type = NotePosTypes::Time;
    if (ntidx >= nobj_by_beat.size()) cur_nobj_type = NotePosTypes::Beat;
    else cur_nobj_type = 
      SeekBySmallerPos(nobj_by_beat[nbidx]->pos.beat, nobj_by_tempo[ntidx]->pos.time_msec);
    if (cur_nobj_type == NotePosTypes::Beat)
    {
      cur_nobj = nobj_by_beat[nbidx];
      nbidx++; 
    } else 
    {
      cur_nobj = nobj_by_beat[ntidx];
      ntidx++;
    }
    switch (cur_nobj->track.subtype)
    {
    case NoteTempoTypes::kBpm:
      SetBPMChange(cur_nobj->value.f);
      break;
    case NoteTempoTypes::kBmsBpm:
      if (c.GetMetaData().GetBPMChannel()->GetBpm(cur_nobj->value.channel, v))
        SetBPMChange(v);
      else
        RPARSER_LOG("Failed to fetch BPM information.");
      break;
    case NoteTempoTypes::kStop:
      SetSTOP(cur_nobj->value.f);
      break;
    case NoteTempoTypes::kBmsStop:
      if (c.GetMetaData().GetSTOPChannel()->GetStop(cur_nobj->value.channel, v))
        SetSTOP(v);
      else
        RPARSER_LOG("Failed to fetch STOP information.");
      break;
      break;
    case NoteTempoTypes::kMeasure:
      SetMeasureLengthChange(cur_nobj->value.f);
      break;
    case NoteTempoTypes::kScroll:
      SetScrollSpeedChange(cur_nobj->value.f);
      break;
    case NoteTempoTypes::kTick:
      SetTick(cur_nobj->value.i);
      break;
    case NoteTempoTypes::kWarp:
      SetWarp(cur_nobj->value.f);
      break;
    }
  }
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
  return tempoobjs_[idx].time_ +
    GetDeltaTimeFromBeatInTempoSegment(tempoobjs_[idx], beat - tempoobjs_[idx].beat_);
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
  return tempoobjs_[idx].beat_ +
    GetDeltaBeatFromTimeInTempoSegment(tempoobjs_[idx], time - tempoobjs_[idx].time_);
}

double TempoData::GetBeatFromRow(const Row& row) const
{
  // binary search to find proper tempoobject segment.
  int min = 0, max = tempoobjs_.size() - 1;
  int l = min, r = max;
  int idx = 0;
  while( l <= r )
  {
    int m = ( l + r ) / 2;
    if( ( m == min || tempoobjs_[m].measure_idx_ <= row.measure ) && ( m == max || row.measure < tempoobjs_[m+1].measure_idx_ ) )
    {
      idx = m;
      break;
    }
    else if( tempoobjs_[m].measure_idx_ > row.measure )
    {
      r = m - 1;
    }
    else
    {
      l = m + 1;
    }
  }

  return GetBeatFromRowInTempoSegment(tempoobjs_[idx], row);
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

std::vector<double> TempoData::GetTimeFromBeatArr(const std::vector<double>& sorted_beat)
{
  std::vector<double> r_time;
  int idx = 0;
  for (double beat: sorted_beat)
  {
    // go to next segment if available.
    if (idx+1 < tempoobjs_.size() && beat > tempoobjs_[idx+1].beat_) idx++;
    r_time.push_back(GetDeltaTimeFromBeatInTempoSegment(tempoobjs_[idx], beat));
  }
  return r_time;
}

std::vector<double> TempoData::GetBeatFromTimeArr(const std::vector<double>& sorted_time)
{
  std::vector<double> r_beat;
  int idx = 0;
  for (double time: sorted_time)
  {
    // go to next segment if available.
    if (idx+1 < tempoobjs_.size() && time > tempoobjs_[idx+1].time_) idx++;
    r_beat.push_back(GetDeltaBeatFromTimeInTempoSegment(tempoobjs_[idx], time));
  }
  return r_beat;
}

std::vector<double> TempoData::GetBeatFromRowArr(const std::vector<Row>& sorted_row)
{
  std::vector<double> r_beat;
  int idx = 0;
  for (const Row& row: sorted_row)
  {
    // go to next segment if available.
    // -- measure pos might be same, so move as much as possible.
    while (idx+1 < tempoobjs_.size() && row.measure > tempoobjs_[idx+1].measure_idx_) idx++;
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
  double beat = tempoobjs_.back().beat_ + GetDeltaBeatFromLastTime(time - tempoobjs_.back().time_);
  TempoObject new_tobj(tempoobjs_.back());
  new_tobj.time_ = time;
  new_tobj.beat_ = beat;
  tempoobjs_.push_back(new_tobj);
}

void TempoData::SeekByBeat(double beat)
{
  ASSERT(beat >= tempoobjs_.back().beat_);
  if (beat == tempoobjs_.back().beat_) return;
  double time = tempoobjs_.back().time_ + GetDeltaBeatFromLastTime(beat - tempoobjs_.back().beat_);
  TempoObject new_tobj(tempoobjs_.back());
  new_tobj.time_ = time;
  new_tobj.beat_ = beat;
  tempoobjs_.push_back(new_tobj);
}

void TempoData::SeekByRow(const Row& row)
{
  // convert row into beat
  const TempoObject& n = tempoobjs_.back();
  const double beat = n.measure_length_changed_beat_
    + n.measure_length_ * (row.measure - n.measure_idx_ + row.num / (double)row.deno);
  SeekByBeat( GetBeatFromRowInTempoSegment(tempoobjs_.back(), row) );
}

NotePosTypes TempoData::SeekBySmallerPos(double beat, double time)
{
  // convert time into beat here to compare.
  double time_to_beat = GetDeltaBeatFromLastTime(time);
  if (beat <= time_to_beat)
  {
    SeekByBeat(beat);
    return NotePosTypes::Beat;
  }
  else
  {
    SeekByTime(time);
    return NotePosTypes::Time;
  }
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
  if (tobj.measure_length_changed_beat_ != tobj.beat_)
  {
    tobj.measure_idx_ = tobj.measure_idx_ +
      (tobj.beat_ - tobj.measure_length_changed_beat_) / tobj.measure_length_;
    tobj.measure_length_changed_beat_ = tobj.beat_;
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
  tempoobjs_.back().warpbeat_ = warp_length_in_beat;
}

void TempoData::SetTick(uint32_t tick)
{
  tempoobjs_.back().tick_ = tick;
}

void TempoData::SetScrollSpeedChange(double scrollspeed)
{
  tempoobjs_.back().scrollspeed_ = scrollspeed;
}

double TempoData::GetDeltaTimeFromLastBeat(double beat_delta)
{
  return GetDeltaTimeFromBeatInTempoSegment(tempoobjs_.back(), beat_delta);
}

double TempoData::GetDeltaBeatFromLastTime(double time_delta_msec)
{
  return GetDeltaBeatFromTimeInTempoSegment(tempoobjs_.back(), time_delta_msec);
}

double TempoData::GetMaxBpm()
{
  double maxbpm = tempoobjs_.front().bpm_;
  for (auto& tobj: tempoobjs_)
  {
    if (maxbpm < tobj.bpm_)
      maxbpm = tobj.bpm_;
  }
  return maxbpm;
}

double TempoData::GetMinBpm()
{
  double minbpm = tempoobjs_.front().bpm_;
  for (auto& tobj: tempoobjs_)
  {
    if (minbpm > tobj.bpm_)
      minbpm = tobj.bpm_;
  }
  return minbpm;
}

bool TempoData::HasBpmChange()
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

bool TempoData::HasStop()
{
  for (auto& tobj: tempoobjs_)
  {
    if (tobj.stoptime_ > 0)
      return true;
  }
  return false;
}

bool TempoData::HasWarp()
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

std::string TempoData::toString()
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
  TempoData();
}

void TempoData::swap(TempoData& tempodata)
{
  std::swap(tempoobjs_, tempodata.tempoobjs_);
}

}
