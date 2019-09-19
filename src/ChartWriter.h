
#ifndef RPARSER_NOTEWRITER_H
#define RPARSER_NOTEWRITER_H

#include "common.h"
#include "Directory.h"

namespace rparser {

class Song;
class Chart;
enum class SONGTYPE;

class ChartWriter {
public:
  ChartWriter();

  /* @brief Write song data, including charts. (automatical) */
  bool Write(Song* s);

  /* @brief Write song metadata file, in case of necessary. */
  virtual bool WriteMeta(const Song* s);

  /* @brief Write charts only */
  virtual bool WriteChart(const Chart* c);

  std::string GetFilename();
protected:
  void AddData(rutil::FileData& d);
private:
  std::string filename_;
  int error;
};

ChartWriter* CreateChartWriter(SONGTYPE songtype);

}

#endif