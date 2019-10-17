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
  
  double beat_;
  double time_;           // in second
  double barpos_;
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
  BarObject(double barpos, double barlength, uint32_t measure);
  BarObject(const BarObject&) = default;

  uint32_t beat_;
  double barpos_;
  double barlength_;
};

/*
 * @detail  Calculates beat/time by chart data.
 * @warn    MUST add object sequentially.
 */
class TimingSegmentData
{
public:
  TimingSegmentData();
  double GetTimeFromBeat(double beat) const;
  double GetBeatFromTime(double time) const;
  double GetBarFromBeat(double beat) const;
  double GetBeatFromBar(double beat) const;
  std::vector<double> GetTimeFromBeatArr(const std::vector<double>& sorted_beat) const;
  std::vector<double> GetBeatFromTimeArr(const std::vector<double>& sorted_time) const;
  std::vector<double> GetBarFromBeatArr(const std::vector<double>& sorted_beat) const;
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
  void Invalidate(const MetaData& m);

private:
  void SetFirstObjectFromMetaData(const MetaData &md);
  void SetMeasureLengthChange(uint32_t measure_idx /* beat */, double measure_length);
  void SeekByBeat(double beat);
  void SeekByTime(double time);
  void SetBPMChange(double bpm);
  void SetSTOP(double stop);
  void SetDelay(double delay);
  void SetWarp(double warp_length_in_beat);
  void SetTick(uint32_t tick);
  void SetScrollSpeedChange(double scrollspeed);
  void Seek(double beat, double time);
  double GetTimeFromBeatInLastSegment(double beat) const;       // return msec time
  double GetBeatFromTimeInLastSegment(double time) const;
  double GetBarFromBeatInLastSegment(double beat) const;

  bool do_recover_measure_length_;        // set measure length to 4.0 implicitly.
  TimingData timingdata_;
  std::vector<TimingSegment> timingsegments_;
  std::vector<BarObject> barobjs_;        // always in sorted state.
};

constexpr double kDefaultMeasureLength = 4.0;

} /* namespace rparser */

#endif
