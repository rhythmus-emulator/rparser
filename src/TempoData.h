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
  double stoptime_;
  double delaytime_;
  double warpbeat_;
  double scrollspeed_;    // Faster scrollspeed marks smaller timepoint for each segment.
  uint32_t tick_;         // XXX: unused (default 4)
  bool is_manipulated_;

  void clearForCopiedSegment();
  std::string toString();
};

/* @detail  Segment used for Bar <--> Beat conversion.
 *          Must proceed first before TempoObject added. */
struct BarObject
{
  BarObject();
  BarObject(double beatpos, double barlength, uint32_t baridx, bool is_implicit);
  BarObject(const BarObject&) = default;

  double beatpos_;
  double barlength_;
  uint32_t baridx_;
  bool is_implicit_;
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
  double GetBeatFromRow(double row) const;
  std::vector<double> GetTimeFromBeatArr(const std::vector<double>& sorted_beat) const;
  std::vector<double> GetMeasureFromBeatArr(const std::vector<double>& sorted_beat) const;
  std::vector<double> GetBeatFromTimeArr(const std::vector<double>& sorted_time) const;
  std::vector<double> GetBeatFromBarArr(const std::vector<double>& sorted_row) const;
  double GetMeasureFromBeat(double beat) const;
  double GetMaxBpm() const;
  double GetMinBpm() const;
  bool HasBpmChange() const;
  bool HasStop() const;
  bool HasWarp() const;
  void SetMeasureLengthRecover(bool recover = true);
  std::string toString() const;
  void clear();
  void swap(TempoData& tempodata);
  NoteData<TempoNote>& GetTempoNoteData();
  const NoteData<TempoNote>& GetTempoNoteData() const;
  void Invalidate(const MetaData& m);

  void SetFirstObjectFromMetaData(const MetaData &md);
  void SetMeasureLengthChange(uint32_t measure_idx, double measure_length);
  void SetMeasureLengthChange(double beat_pos, double measure_length);
  void SeekByBeat(double beat);
  void SeekByTime(double time);
  void SeekByBar(double bar);
  void SetBPMChange(double bpm);
  void SetSTOP(double stop);
  void SetDelay(double delay);
  void SetWarp(double warp_length_in_beat);
  void SetTick(uint32_t tick);
  void SetScrollSpeedChange(double scrollspeed);
private:
  void Seek(double beat, double time);
  double GetTimeFromBeatInLastSegment(double beat) const;       // return msec time
  double GetBeatFromTimeInLastSegment(double time) const;
  double GetBeatFromRowInLastSegment(double measure) const;

  bool do_recover_measure_length_;        // set measure length to 4.0 implicitly.
  NoteData<TempoNote> temponotedata_;
  std::vector<TempoObject> tempoobjs_;
  std::vector<BarObject> barobjs_;        // always in sorted state.
};

constexpr double kDefaultMeasureLength = 4.0;

} /* namespace rparser */

#endif
