#ifndef RPARSER_TEMPODATA_H
#define RPARSER_TEMPODATA_H

#include "common.h"
#include "Note.h"

namespace rparser
{

class MetaData;

/* @detail  Segment object affecting chart tempo.
 *          Must be scanned sequentially by beat/time. */
struct TempoObject
{
  TempoObject();
  TempoObject(const TempoObject&) = default;
  
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

  void clearForCopiedSegment();
  std::string toString();
};

/*
 * @detail  Calculates beat/time by chart data.
 * @warn    MUST add object sequentially.
 */
class TempoData
{
public:
  TempoData();
  double GetTimeFromBeat(double beat) const;
  double GetBeatFromTime(double time) const;
  double GetBeatFromRow(const Row& row) const;
  std::vector<double> GetTimeFromBeatArr(const std::vector<double>& sorted_beat) const;
  std::vector<double> GetBeatFromTimeArr(const std::vector<double>& sorted_time) const;
  std::vector<double> GetBeatFromRowArr(const std::vector<Row>& sorted_row) const;
  double GetMeasureFromBeat(double beat) const;
  double GetMaxBpm() const;
  double GetMinBpm() const;
  bool HasBpmChange() const;
  bool HasStop() const;
  bool HasWarp() const;
  std::string toString() const;
  void clear();
  void swap(TempoData& tempodata);
  NoteData<TempoNote>& GetTempoNoteData();
  void Invalidate(const MetaData& m);

  void SetFirstObjectFromMetaData(const MetaData &md);
  void SeekByTime(double time);
  void SeekByBeat(double beat);
  void SeekByRow(const Row& row);
  void SetBPMChange(double bpm);
  void SetMeasureLengthChange(double measure_length);
  void SetSTOP(double stop);
  void SetDelay(double delay);
  void SetWarp(double warp_length_in_beat);
  void SetTick(uint32_t tick);
  void SetScrollSpeedChange(double scrollspeed);
private:
  double GetTimeFromBeatInLastSegment(double beat) const;       // return msec time
  double GetBeatFromTimeInLastSegment(double time) const;
  double GetBeatFromRowInLastSegment(const Row& row) const;

  NoteData<TempoNote> temponotedata_;
  std::vector<TempoObject> tempoobjs_;
};

} /* namespace rparser */

#endif
