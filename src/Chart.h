/*
 * by @lazykuna, MIT License.
 */
 
#ifndef RPARSER_CHART_H
#define RPARSER_CHART_H

#include "ChartData.h"
#include "MetaData.h"
#include "TimingData.h"
#include <sstream>

namespace rparser {
/*
 * @description
 * chart data contains: notedata, timingdata, metadata
 * includes mutable note objects.
 */
class Chart
{
public:
	bool Open(const char* filepath);
	bool Load(const char* data, unsigned int len);

    const MetaData* GetMetaData() const;
    const ChartData* GetChartData() const;
    const TimingData* GetTimingData() const;
    MetaData* GetMetaData();
	ChartData* GetChartData();
    TimingData* GetTimingData();

	void SetExternMetaData(MetaData* md);
	void SetExternTimingData(TimingData* td);
	void SetIndepMetaData();
	void SetIndepTimingData();

    // @description clone myself to another chart
    Chart* Clone();

    // @description
	// ONLY call this function when timing related object is changed.
	// (e.g. when exiting editing mode and entering view mode)
    // Calling this function requires full scan of NoteData, so be aware of it ...
    void UpdateTimingData();

    std::string toString();
    Chart(const TimingData *td=0, const MetaData *md=0);
    Chart(const Chart* c);
private:
    ChartData chartdata_;
    TimingData timingdata_;

    // chart status structure
	// (not saved)
	struct ChartStatus {
		std::string filepath;	// absolute path
		std::string relpath;	// relative path
		char hash[32];			// MD5 hash of chart file
	} chartstat_;
};

}

#endif