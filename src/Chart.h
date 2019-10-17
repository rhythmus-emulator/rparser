/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTDATA_H
#define RPARSER_CHARTDATA_H

#include "common.h"
#include "Note.h"
#include "TempoData.h"
#include "MetaData.h"
#include "rutil.h"

namespace rparser
{

class ConditionalChart;
class Song;
enum class SONGTYPE;

/*
 * @detail
 * Contains all object consisting chart.
 *
 * @params
 * notedata_ notedata containing all types of notes
 *           (including special/timing related note objects)
 * metadata_ metadata to be used (might be shared with other charts)
 * timingdata_ timingdata to be used (might be shared with other charts)
 * hash_ hash value of chart file. (Updated by ChartWriter / ChartLoader)
 */
class Chart
{
/* iterators */
public:
  Chart();
  Chart(const Chart &nd);
  ~Chart();

  BgmData& GetBgmData();
  BgaData& GetBgaData();
  NoteData& GetNoteData();
  EffectData& GetEffectData();
  TimingData& GetTimingData();
  TimingSegmentData& GetTimingSegmentData();
  MetaData& GetMetaData();
  const MetaData& GetMetaData() const;

  uint32_t GetScoreableNoteCount() const;
  double GetSongLastObjectTime() const;
  bool HasLongnote() const;
  uint8_t GetPlayLaneCount() const;
  
  void Clear();

  void swap(Chart& c);

  virtual std::string toString() const;

  void InvalidateAllNotePos();
  void InvalidateNotePos(Note &n);
  void InvalidateTempoData();
  void Invalidate();

  bool IsEmpty();
  std::string GetHash() const;
  void SetHash(const std::string& hash);
  std::string GetFilename() const;
  void SetFilename(const std::string& filename);
  void SetParent(Song *song);
  SONGTYPE GetSongType() const;

private:
  BgmData bgmdata_;
  BgaData bgadata_;
  NoteData notedata_;
  EffectData effectdata_;
  MetaData metadata_;
  TimingSegmentData timingsegmentdata_;
  Song* parent_song_;
  std::string hash_;
  std::string filename_;
};

/**
 * @detail
 * Struct to access or modify chart data
 *
 * @param
 * size Get total available chart data count.
 * AddNewChart Add new chart and return current new index of chart.
 * GetChartData Get idx chart data (read-only).
 *              return 0 if invalid index or cannot make new context (previous one not closed).
 * CloseChartData Close and apply move edited data to original chart.
 * DeleteChart Delete idx chart.
 * UpdateTempoData Update tempo data of all charts.
 *
 * \warn
 * Chart data MUST BE CLOSED before edit another one.
 * Only one chart object is capable for read/edit, originally.
 */
class ChartListBase
{
public:
  virtual ~ChartListBase() = default;
  virtual size_t size() = 0;
  virtual int AddNewChart() = 0;
  virtual const Chart* GetChartData(int idx) const = 0;
  virtual Chart* GetChartData(int idx) = 0;
  virtual void CloseChartData() = 0;
  virtual void DeleteChart(int idx) = 0;
  virtual bool IsChartOpened() = 0;
  virtual void UpdateTempoData() = 0;
  virtual int GetChartIndexByName(const std::string& filename) = 0;
protected:
  ChartListBase() = default;
};

/**
* @detail
* Contains multiple Chart object without common objects (tempo, BGA ...)
* E.g. BMS file format.
*/
class ChartList: public ChartListBase
{
public:
  ChartList();
  virtual ~ChartList();
  virtual size_t size();
  virtual int AddNewChart();
  virtual const Chart* GetChartData(int idx) const;
  virtual Chart* GetChartData(int idx);
  virtual void CloseChartData();
  virtual void DeleteChart(int idx);
  virtual bool IsChartOpened();
  virtual void UpdateTempoData();
  virtual int GetChartIndexByName(const std::string& filename);

  void AppendChart(Chart* chart);
  Chart* GetChart(int idx);
private:
  std::vector<Chart*> charts_;
  mutable int cur_edit_idx;
};

/**
 * @detail
 * Contains multiple difficulty charts with single time object. Shares Tempo/BGA channel.
 * which is suitable for SM/OSU type format.
 */
class ChartNoteList: private Chart, public ChartListBase
{
public:
  ChartNoteList();
  virtual ~ChartNoteList();
  virtual size_t size();
  virtual int AddNewChart();
  virtual const Chart* GetChartData(int idx) const;
  virtual Chart* GetChartData(int idx);
  virtual void CloseChartData();
  virtual void DeleteChart(int idx);
  virtual bool IsChartOpened();
  virtual void UpdateTempoData();
  virtual int GetChartIndexByName(const std::string& filename);
private:
  std::vector< NoteData > note_charts_;
  mutable int cur_edit_idx;
};

} /* namespace rparser */

#endif
