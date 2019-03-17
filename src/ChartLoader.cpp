#include "ChartLoader.h"

namespace rparser {

void ChartLoader::SetChart(Chart *c) { chart_ = c; chartlist_ = 0; }

void ChartLoader::SetChartList(ChartListBase *chartlist) { chart_ = 0; chartlist_ = chartlist; }

}