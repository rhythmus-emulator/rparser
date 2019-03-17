
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
  bool SerializeCommonData(const ChartListBase& chartlist, Resource& r);
  Resource::BinaryData& GetData();
protected:
  void AddData(Resource::BinaryData& d);
private:
  Resource::BinaryData data_cache_;
  int error;
};

ChartWriter* CreateChartWriter(SONGTYPE songtype);

}

#endif