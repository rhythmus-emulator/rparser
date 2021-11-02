#ifndef RPARSER_CHARTUTIL_H
#define RPARSER_CHARTUTIL_H

#include "Chart.h"

namespace rparser
{

constexpr int kMaxSizeLane = 20;

/**
 * Export chart data (metadata, notedata, events etc ...) to HTML format.
 * CSS and other necessary things should be made by oneself ...
 */
class ChartExporter
{
public:
  ChartExporter();
  ChartExporter(const Chart& c);
  const std::string& toHTML();
private:
  void Analyze(const Chart& c);
  void GenerateHTML(const Chart& c);
  struct Measure
  {
    double length;
    std::vector<std::pair<unsigned, const NoteElement*> > nd_;
    std::vector<std::pair<unsigned, const NoteElement*> > td_;
  };
  std::vector<Measure> measures_;
  std::string html_;
};

namespace effector
{
  enum LaneTypes
  {
    LOCKED = 0,
    NOTE,
    SC,
    PEDAL
  };

  struct EffectorParam
  {
    int player;                     // default is 0 (1P)
    int lanesize;                   // default is 7
    int lockedlane[kMaxSizeLane];   // flag for locked lanes for changing (MUST sorted); ex: SC, pedal, ...
    int sc_lane;                    // scratch lane idx (for AllSC option)
    int seed;
  };

  void SetLanefor7Key(EffectorParam& param);
  void SetLanefor9Key(EffectorParam& param);
  void SetLaneforBMS1P(EffectorParam& param);
  void SetLaneforBMS2P(EffectorParam& param);
  void SetLaneforBMSDP1P(EffectorParam& param);
  void SetLaneforBMSDP2P(EffectorParam& param);

  /**
   * @detail Generate random column mapping. 
   * @params
   * cols           integer array going to be written lane mapping.
   * EffectorParam  contains locked lane info and lanesize to generate random mapping.
   */
  void GenerateRandomColumn(int *cols, const EffectorParam& param);
  void Random(Chart &c, const EffectorParam& param);
  void SRandom(Chart &c, const EffectorParam& param);
  void HRandom(Chart &c, const EffectorParam& param);
  void RRandom(Chart &c, const EffectorParam& param, bool shift_by_timing=false);
  void Mirror(Chart &c, const EffectorParam& param);
  void AllSC(Chart &c, const EffectorParam& param);
  void Flip(Chart &c, const EffectorParam& param);
}

template <typename T>
struct RowElement
{
public:
  std::vector<std::pair<unsigned, T*> > notes;
  double pos;
  NoteElement* get(size_t column);
  const NoteElement* get(size_t column) const;
};

/* for note iterating by row
 * XXX: Do not edit NoteData while using RowIterator. may access to corrupted memory. */
template <typename TD, typename T>
class RowElementCollection
{
public:
  RowElementCollection(TD& td);
  RowElementCollection(TD& td, double m_start, double m_end);
  typename std::vector<RowElement<T> >::iterator begin();
  typename std::vector<RowElement<T> >::iterator end();
  typename std::vector<RowElement<T> >::const_iterator begin() const;
  typename std::vector<RowElement<T> >::const_iterator end() const;
private:
  std::vector<RowElement<T> > rows_;
};

typedef RowElementCollection<TrackData, NoteElement> RowCollection;
typedef RowElementCollection<const TrackData, const NoteElement> ConstRowCollection;

struct ProfileSegment
{
  double time;
  double delta_time;
  double pos;
  double delta_pos;
  float bpm;
  unsigned notes;
  std::string pattern;
  uint32_t pattern_i[2];
};

class ChartProfiler
{
public:
  ChartProfiler();
  ChartProfiler(const Chart& c);
  void Profile(const Chart& c);
  std::string toString() const;
  void Save(const std::string& path) const;

private:
  std::string name_;
  std::vector<ProfileSegment> row_segments_;
  int stop_count_;
  int bpm_change_;
};

}

#endif