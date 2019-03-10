
#ifndef RPARSER_NOTEWRITER_H
#define RPARSER_NOTEWRITER_H

#include "common.h"
#include "Chart.h"
#include "Resource.h"

namespace rparser {

class ChartWriter {
public:
  ChartWriter();
  bool Serialize(const Chart& c);
  bool SerializeCommonData(const NoteData& objdata, const MetaData& metadata);
  typedef std::vector<Resource::BinaryData>::iterator data_iter;
  data_iter data_begin();
  data_iter data_end();
protected:
  void AddData(Resource::BinaryData& d);
private:
  std::vector<Resource::BinaryData> datas_;
  int error;
};

ChartWriter* GetChartWriter(SONGTYPE songtype);

}

#endif