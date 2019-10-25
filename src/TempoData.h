#ifndef RPARSER_TEMPODATA_H
#define RPARSER_TEMPODATA_H

#include "common.h"
#include "Note.h"

namespace rparser
{

class MetaData;
using TimingData = TrackDataWithType<TimingObject>;

/* @detail  Segment object affecting chart tempo.
 *          Must be scanned sequentially by beat/time. */
struct TimingSegment
{
  TimingSegment();
  TimingSegment(const TimingSegment&) = default;
  
  double time_;           // in second
  double beat_;           // real object position used for time calculation (4/4 beat by default)
  double measure_;        // edit object position (only for debug, not used)
  double bpm_;
  double stoptime_;
  double delaytime_;
  double warpbeat_;
  double scrollspeed_;    // Faster scrollspeed marks smaller timepoint for each segment.
  uint32_t tick_;         // XXX: unused (default 4)
  bool is_manipulated_;

  void clearForCopiedSegment();
  std::string toString();
};

/**
 * @detail
 * Segment used for Bar <--> Beat conversion.
 * Bar position: actual display position with scroll segment.
 *
 * @warn
 * Data must proceed first before Timing calculation.
 * (barlength effects timing calculation)
 */
struct BarObject
{
  BarObject();
  BarObject(double beat, double barlength, uint32_t measure);
  BarObject(const BarObject&) = default;
  bool operator<(const BarObject &) const;

  uint32_t measure_;
  double beat_;
  double barlength_;    // 0 ~ 1
};

/*
 * @detail  Calculates beat/time by chart data.
 * @warn    MUST add object sequentially.
 */
class TimingSegmentData
{
public:
  TimingSegmentData();
  double GetTimeFromMeasure(double measure) const;
  double GetMeasureFromTime(double time) const;
  double GetTimeFromBeat(double beat) const;
  double GetBeatFromTime(double time) const;
  double GetBeatFromMeasure(double measure) const;
  double GetMeasureFromBeat(double beat) const;
  std::vector<double> GetTimeFromMeasureArr(const std::vector<double>& sorted_measure) const;
  std::vector<double> GetMeasureFromTimeArr(const std::vector<double>& sorted_time) const;
  std::vector<double> GetBeatFromMeasureArr(const std::vector<double>& sorted_measure) const;
  double GetMaxBpm() const;
  double GetMinBpm() const;
  bool HasBpmChange() const;
  bool HasStop() const;
  bool HasWarp() const;
  void SetMeasureLengthRecover(bool recover = true);
  std::string toString() const;
  void clear();
  void swap(TimingSegmentData& timingdata);
  TimingData &GetTimingData();
  const TimingData &GetTimingData() const;
  const std::vector<BarObject>& GetBarObjects() const;
  double GetBarLength(uint32_t measure) const;
  void Invalidate(const MetaData& m);

private:
  void SetFirstObjectFromMetaData(const MetaData &md);
  void SetMeasureLengthChange(uint32_t measure_idx /* beat */, double measure_length);
  void SeekByMeasure(double measure);
  void SeekByTime(double time);
  void Seek(double beat, double time);
  void SetBPMChange(double bpm);
  void SetSTOP(double stop);
  void SetDelay(double delay);
  void SetWarp(double warp_length_in_beat);
  void SetTick(uint32_t tick);
  void SetScrollSpeedChange(double scrollspeed);
  double GetTimeFromBeatInLastSegment(double beat) const;       // return msec time
  double GetBeatFromTimeInLastSegment(double time) const;
  double GetBeatFromMeasureInLastSegment(double measure) const;
  double GetMeasureFromBeatInLastSegment(double beat) const;
  double GetTimeFromMeasureInLastSegment(double measure) const;
  double GetMeasureFromTimeInLastSegment(double time) const;

  bool do_recover_measure_length_;        // set measure length to 4.0 implicitly.
  TimingData timingdata_;
  std::vector<TimingSegment> timingsegments_;
  std::vector<BarObject> barobjs_;        // always in sorted state.
};

constexpr double kDefaultMeasureLength = 4.0;

} /* namespace rparser */

#endif
