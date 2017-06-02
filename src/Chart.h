/*
 * by @lazykuna, MIT License.
 */
 
#ifndef RPARSER_CHART_H
#define RPARSER_CHART_H

#include "NoteData.h"
#include "MetaData.h"
#include "TimingData.h"

namespace rparser {

enum class CHARTTYPE {
	UNKNOWN,
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
 */
class Chart {
public:
    MetaData* GetMetaData() const;
    NoteData* GetNoteData() const;
    TimingData* GetTimingData() const;
    MetaData* GetMetaData();
    NoteData* GetNoteData();
    TimingData* GetTimingData();
    const ChartMetaData* GetChartSummary() const;  // cannot modify, read-only
    std::string GetFilePath() const;
    void UpdateChartSummary();

    // @description clone myself to another chart
    Chart* Clone();

    // @description read & write utilities
    int Read(const std::string& path);
    int Read(const char* p, int iLen);
    int Write(const std::string& path);
    int Write(std::stream& s);

    // @description change resolution for chart globally
    void ChangeResolution(int newRes);
private:
    MetaData m_Metadata;
    NoteData m_Notedata;
    TimingData m_Timingdata;

    // not saved to file, just to store current chart state.
    ChartSummaryData m_ChartSummaryData;
};




/*
 * @description
 * only contains brief chart metadata, which may be useful for visualizing status.
 * Also these status won't be saved.
 */
struct ChartSummaryData {
    std::string sFilePath;
    std::string sFormat;
    std::string sHash;      // MD5
    CHARTTYPE iFormat;      // chart file type

    int iNoteCount;
    float fLastNoteTime;    // msec

    // @description basc BPM of the song
    int iBPM;
    // @description is bpm changes during song playing?
    bool isBPMChanges;
    int iMaxBPM;
    int iMinBPM;
    // @description is bomb object exists?
    bool isBomb;
    // @description is wrap object(negative bpm) exists?
    bool isWrap;
    bool isSTOP;
    // @description is backspin scratch exists?
    bool isBSS;
    bool isCharge;
    bool isHellCharge;
    // @description is command exists/processed? (in case of BMS)
    bool isCommand;
    
    void FillHash(const char* p, int iLen);
    void Fill(const Chart& chart);
};


}

#endif