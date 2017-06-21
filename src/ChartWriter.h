
#ifndef RPARSER_NOTEWRITER_H
#define RPARSER_NOTEWRITER_H

#include "Chart.h"

namespace rparser {

class ChartWriter {
protected:
    const Chart *c;
    int error;
public:
    ChartWriter(const Chart *c) : c(c), error(0) {  }
    virtual int Write() = 0;
};

int WriteChart(const Chart *c, const std::string &fn);

}

#endif