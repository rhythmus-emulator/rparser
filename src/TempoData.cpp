#include "TempoData.h"
#include "MetaData.h"
#include "ChartUtil.h"

namespace rparser
{

inline double GetTimeFromBeatInTempoSegment(const TempoObject& tobj, double beat)
{
  const double msec_per_beat = 60.0 * 1000 / tobj.bpm_;
  return tobj.time_ + (beat - tobj.beat_) * msec_per_beat;
}

inline double GetBeatFromTimeInTempoSegment(const TempoObject& tobj, double time_msec)
{
  const double beat_per_msec = tobj.bpm_ / 60 / 1000;
  return tobj.beat_ + (time_msec - tobj.time_) * beat_per_msec;
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
  // Dummy object to avoid crashing
  tempoobjs_.push_back(TempoObject());
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
  // extract tempo related object from chartdata
  static const NotePosTypes nbtypes[3] = { NotePosTypes::Beat, NotePosTypes::Time, NotePosTypes::Row };
  std::vector<const Note*> nobj_by_beat, nobj_by_tempo, nobj_by_row;
  std::vector<const Note*> *nobjvecs[] = { &nobj_by_beat, &nobj_by_tempo, &nobj_by_row };
  int nbidx[3];
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
  while (nbidx[0] < nobj_by_beat.size() &&
         nbidx[1] < nobj_by_tempo.size() &&
         nbidx[2] < nobj_by_row.size())
  {
    // select tempo note which is in most early timing.
    if (nbidx[0] >= nobj_by_beat.size()) { nbarr[0] = DBL_MAX; }
    else { nbarr[0] = nobj_by_beat[nbidx[0]]->pos.beat; }

    if (nbidx[1] >= nobj_by_tempo.size()) { nbarr[1] = DBL_MAX; }
    else { nbarr[1] = GetBeatFromTimeInLastSegment(nobj_by_tempo[nbidx[1]]->pos.time_msec); }

    if (nbidx[2] >= nobj_by_row.size()) { nbarr[2] = DBL_MAX; }
    else { nbarr[2] = GetBeatFromRowInLastSegment(nobj_by_row[nbidx[2]]->pos.row); }

    cur_nobj_type_idx = GetSmallestIndex(nbarr, 3);
    cur_nobj = static_cast<const TempoNote*>( (*nobjvecs[cur_nobj_type_idx])[ nbidx[cur_nobj_type_idx] ] );

    // seek for next tempo segment object and update note object beat value.
    // (exceptionally use const_cast for update beat position)
    SeekByBeat(nbarr[cur_nobj_type_idx]);
    const_cast<TempoNote*>(cur_nobj)->pos.beat = nbarr[cur_nobj_type_idx];

    // set tempo segment object attribute.
    switch (cur_nobj->subtype)
    {
    case NoteTempoTypes::kBpm:
      SetBPMChange(cur_nobj->value.f);
      break;
    case NoteTempoTypes::kBmsBpm:
      if (m.GetBPMChannel()->GetBpm(cur_nobj->value.i, v))
        SetBPMChange(v);
      else
        RPARSER_LOG("Failed to fetch BPM information.");
      break;
    case NoteTempoTypes::kStop:
      SetSTOP(cur_nobj->value.f);
      break;
    case NoteTempoTypes::kBmsStop:
      if (m.GetSTOPChannel()->GetStop(cur_nobj->value.i, v))
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

  // sort internal temponotedata internally by beat.
  std::sort(temponotedata_.begin(), temponotedata_.end());
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
    GetTimeFromBeatInTempoSegment(tempoobjs_[idx], beat);
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
    GetBeatFromTimeInTempoSegment(tempoobjs_[idx], time - tempoobjs_[idx].time_);
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

std::vector<double> TempoData::GetTimeFromBeatArr(const std::vector<double>& sorted_beat) const
{
  std::vector<double> r_time;
  int idx = 0;
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
  int idx = 0;
  for (double time: sorted_time)
  {
    // go to next segment if available.
    if (idx+1 < tempoobjs_.size() && time > tempoobjs_[idx+1].time_) idx++;
    r_beat.push_back(GetBeatFromTimeInTempoSegment(tempoobjs_[idx], time));
  }
  return r_beat;
}

std::vector<double> TempoData::GetBeatFromRowArr(const std::vector<Row>& sorted_row) const
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
  double beat = GetBeatFromTimeInLastSegment(time);
  TempoObject new_tobj(tempoobjs_.back());
  new_tobj.time_ = time;
  new_tobj.beat_ = beat;
  tempoobjs_.push_back(new_tobj);
}

void TempoData::SeekByBeat(double beat)
{
  ASSERT(beat >= tempoobjs_.back().beat_);
  if (beat == tempoobjs_.back().beat_) return;
  double time = GetBeatFromTimeInLastSegment(beat);
  TempoObject new_tobj(tempoobjs_.back());
  new_tobj.time_ = time;
  new_tobj.beat_ = beat;
  tempoobjs_.push_back(new_tobj);
}

void TempoData::SeekByRow(const Row& row)
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

double TempoData::GetTimeFromBeatInLastSegment(double beat_delta) const
{
  return GetTimeFromBeatInTempoSegment(tempoobjs_.back(), beat_delta);
}

double TempoData::GetBeatFromTimeInLastSegment(double time_delta_msec) const
{
  return GetBeatFromTimeInTempoSegment(tempoobjs_.back(), time_delta_msec);
}

double TempoData::GetBeatFromRowInLastSegment(const Row& row) const
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
  TempoData();
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
