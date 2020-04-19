/*
 * by @lazykuna, MIT License.
 */

#ifndef RPARSER_CHARTDATA_H
#define RPARSER_CHARTDATA_H

#include "Note.h"
#include "TempoData.h"
#include "MetaData.h"
#include "rutil.h"

namespace rparser
{

class ConditionalChart;
class Song;
enum class SONGTYPE;

enum class CHARTTYPE
{
  None,
  Chart4Key,
  Chart5Key,
  Chart6Key,
  Chart7Key,
  Chart8Key,
  Chart9Key,
  Chart10Key,
  IIDX5Key,
  IIDX10Key,
  IIDXSP,
  IIDXDP,
  Popn,
  Drummania,
  Guitarfreaks,
  jubeat,
  DDR,
  DDR_DP,
  Pump,
  Pump_DP,
  SDVX,
};

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

  TimingSegmentData& GetTimingSegmentData();
  MetaData& GetMetaData();
  const TimingSegmentData& GetTimingSegmentData() const;
  const MetaData& GetMetaData() const;
  TrackData& GetBgmData();
  TrackData& GetNoteData();
  TrackData& GetCommandData();
  TrackData& GetTimingData();
  const TrackData& GetBgmData() const;
  const TrackData& GetNoteData() const;
  const TrackData& GetCommandData() const;
  const TrackData& GetTimingData() const;

  uint32_t GetScoreableNoteCount() const;
  double GetSongLastObjectTime() const;
  bool HasLongnote() const;
  unsigned GetPlayLaneCount() const;
  
  void Clear();

  void swap(Chart& c);

  virtual std::string toString() const;

  void UpdateAllNotePos();
  void UpdateNotePos(NoteElement &n);
  void UpdateTempoData();
  void UpdateCharttype();
  void Update();

  bool IsEmpty();
  std::string GetFilename() const;
  void SetFilename(const std::string& filename);
  void SetParent(Song *song);
  Song* GetParent() const;
  CHARTTYPE GetChartType() const;
  int GetKeycount() const;

  /**
   * @detail
   * This hash is generated for identity key of the chart.
   * OpenSSL MD5 hash is used to generate this key
   * (otherwise dummy key will created, which is useless for identity code)
   * The key will be updated when chart is load or saved
   * by ChartReader / ChartWriter.
   */
  const std::string &GetHash() const;

  /* @brief seed value used for this chart. */
  int GetSeed() const;

  friend class Song;
  friend class ChartWriter;
  friend class ChartLoader;

private:
  TrackData trackdata_[TrackTypes::kTrackMax];
  MetaData metadata_;
  TimingSegmentData timingsegmentdata_;
  struct {
    TrackData *trackdata[TrackTypes::kTrackMax];
    TimingSegmentData *timingsegmentdata;
  } shared_data_;
  Song* parent_song_;
  int seed_;
  std::string hash_;
  std::string filename_;
  CHARTTYPE charttype_;
};

} /* namespace rparser */

#endif
