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
  const BgmData& GetBgmData() const;
  const BgaData& GetBgaData() const;
  const NoteData& GetNoteData() const;
  const EffectData& GetEffectData() const;
  const TimingData& GetTimingData() const;
  const TimingSegmentData& GetTimingSegmentData() const;
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

  friend class Song;

private:
  BgmData bgmdata_;
  BgaData bgadata_;
  NoteData notedata_;
  EffectData effectdata_;
  MetaData metadata_;
  TimingSegmentData timingsegmentdata_;
  struct {
    BgmData *bgmdata_;
    BgaData *bgadata_;
    EffectData *effectdata_;
    TimingSegmentData *timingsegmentdata_;
  } common_data_;
  Song* parent_song_;
  std::string hash_;
  std::string filename_;
};

} /* namespace rparser */

#endif
