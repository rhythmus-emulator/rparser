/*
 * by @lazykuna, MIT License.
 *
 * Song loader / saver, representing rparser library.
 * This library has functions as follows:
 * - Load and Save
 * - Edit
 * - Digest (in mixable form)
 * - Resource managing (binary form)
 */
 
#ifndef RPARSER_SONG_H
#define RPARSER_SONG_H

#include "Directory.h"
#include "Chart.h"
#include "Error.h"

#include "common.h"
#include "rutil.h"

namespace rparser {

// MUST be in same order with Chart::CHARTTYPE
enum class SONGTYPE {
  NONE = 0,
  BMS,
  BMSON,
  OSU,
  VOS,
  SM,
  DTX,
  OJM,
};

/**
 * @brief
 * Contains song chart, metadata and all resource(image, sound).
 *
 * @param 
 * resource_ contains all song objects
 * chartlist_ contains charts.
 * songtype_ (not saved) detected chart format of this song
 * error_ last error status
 * SetSongType create, or change song type into another type.
 * GetChartCount get total available chart count
 * NewChart create new chart
 * GetChart get chart of idx. return 0 if inavailable.
 * CloseChart close chart handle of currently editing.
 * DeleteChart delete chart of idx.
 * Open read song file (including chart and other metadata, resources.)
 *      set onlyreadchart true for fast loading.
 * Save save all changes.
 *      warning: MUST close chart file before process save.
 * Close close song file and empty handle.
 */
class Song {
public:
  Song();
  ~Song();

  size_t GetChartCount();
  Chart* NewChart();
  Chart* GetChart(int i = 0);
  Chart* GetChart(const std::string& filename);
  void CloseChart();
  void DeleteChart(int i);

  bool SetSongType(SONGTYPE songtype);
  SONGTYPE GetSongType() const;

  bool Open(const std::string &path, SONGTYPE songtype = SONGTYPE::NONE);
  bool Save();
  bool SaveChart(Chart *chart);
  bool SaveAs(const std::string& newpath);
  bool Close(bool save = false);

  const std::string GetPath() const;
  const char* GetErrorStr() const;
  Directory* GetDirectory();

  virtual std::string toString(bool detailed=false) const;

private:
  SONGTYPE DetectSongtype();

  std::string filepath_;
  std::shared_ptr<Directory> directory_;
  SONGTYPE songtype_;
  ERROR error_;
  ChartListBase* chartlist_;

  // in case of need ...
  std::string errormsg_detailed_;

  // to check whether song type is filetype or dirtype
  static bool IsSongExtensionIsFiletype(const std::string& path);
};

SONGTYPE GetSongTypeByName(const std::string& filename);
const char* GetExtensionBySongType(SONGTYPE iType);

}

#endif
