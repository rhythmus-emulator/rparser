
#ifndef RPARSER_NOTEWRITER_H
#define RPARSER_NOTEWRITER_H

#include "common.h"
#include "Chart.h"
#include "Directory.h"

namespace rparser {

class ChartWriter {
public:
  ChartWriter();
  bool Serialize(const Chart& c);
  bool SerializeCommonData(const ChartListBase& chartlist);
  rutil::FileData& GetData();
  std::string GetFilename();
  bool IsWritable();
protected:
  void AddData(rutil::FileData& d);
private:
  rutil::FileData data_cache_;
  std::string filename_;
  bool is_writable_;
  int error;
};

ChartWriter* CreateChartWriter(SONGTYPE songtype);

}

#endif