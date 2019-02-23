#ifndef RPARSER_MIXINGDATA_H
#define RPARSER_MIXINGDATA_H

#include "common.h"

namespace rparser
{

/* @detail  Segment object affecting chart tempo.
 *          Must be scanned sequentially by beat/time. */
struct TempoObject
{
  TempoObject();
  TempoObject(const TempoObject&);
  
  double beat_;
  double time_;           // in second
  double bpm_;
  double measure_idx_;
  double measure_length_changed_beat_;
  double measure_length_; // default: 4 beat 1 measure
  double stoptime_;
  double delaytime_;
  double warpbeat_;
  double scrollspeed_;    // XXX: scroll speed is not available here.
  uint32_t tick_;         // XXX: unused (default 4)

  std::string toString();
};

struct NotePos;
struct NotePos::Row;

/*
 * @detail  Calculates beat/time by chart data.
 * @warn    MUST add object sequentially.
 */
class TempoData
{
public:
  TempoData();
  TempoData(const Chart& chart);
  double GetTimeFromBeat(double beat) const;
  double GetBeatFromTime(double time) const;
  double GetBeatFromRow(const NotePos::Row& row) const;
  std::vector<double> GetTimeFromBeatArr(const std::vector<double>& sorted_beat);
  std::vector<double> GetBeatFromTimeArr(const std::vector<double>& sorted_time);
  std::vector<double> GetBeatFromRowArr(const std::vector<NotePos::Row>& sorted_row);
  double GetMeasureFromBeat(double beat) const ;
  double GetMaxBpm();
  double GetMinBpm();
  bool HasBpmChange();
  bool HasStop();
  bool HasWarp();
  std::string toString();
  void clear();
  void swap(TempoData& tempodata);
private:
  void SetFirstObjectFromMetaData(const MetaData &md);
  void SeekByTime(double time);
  void SeekByBeat(double beat);
  void SeekByRow(const NotePos::Row& row);
  NotePosTypes SeekBySmallerPos(double beat, double time);
  void SetBPMChange(double bpm);
  void SetMeasureLengthChange(double measure_length);
  void SetSTOP(double stop);
  void SetDelay(double delay);
  void SetWarp(double warp_length_in_beat);
  void SetTick(uint32_t tick);
  void SetScrollSpeedChange(double scrollspeed);
  double GetDeltaTimeFromLastBeat(double beat_delta);       // return msec time
  double GetDeltaBeatFromLastTime(double time_delta_msec);

  std::vector<TempoObject> tempoobjs_;
};

} /* namespace rparser */

#endif
