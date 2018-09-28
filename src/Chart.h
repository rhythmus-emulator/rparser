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

// MUST be in same order with Song::SONGTYPE
enum class CHARTTYPE {
	NONE = 0,
	BMS,
	BMSON,
	OSU,
	VOS,
	SM,
	DTX,
	OJM,
};

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

    std::string GetFilePath() const;
	void SetFilePath(const std::string& sPath);

    // @description clone myself to another chart
    Chart* Clone();

    // @description
	// ONLY call this function when timing related object is changed.
	// (e.g. when exiting editing mode and entering view mode)
    // Calling this function requires full scan of NoteData, so be aware of it ...
    void UpdateTimingData();

	// generate mixing object
	void GenerateMixingData(MixingData& md);
	// returns chart object file to iLen;
	bool Flush(char **out, int &iLen) const;
	// returns true if there is any modification from last Flush()
	bool IsDirty() const;


    std::string toString();
    Chart();
    Chart(Chart* c);
private:
    ChartData chartdata_;
    TimingData timingdata_;

    // chart status structure
	// (not saved)
	struct ChartStatus {
		std::string filepath;	// absolute path
		std::string relpath;	// relative path
		char hash[33];			// MD5 hash of chart file (len 32)
		bool dirty;				// is it modified from last Flush() ?
		CHARTTYPE type;			// chart file format
	} chartstat_;
};

}

#endif