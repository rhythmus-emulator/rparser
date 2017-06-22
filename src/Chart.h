/*
 * by @lazykuna, MIT License.
 */
 
#ifndef RPARSER_CHART_H
#define RPARSER_CHART_H

#include "NoteData.h"
#include "MetaData.h"
#include "TimingData.h"
#include <sstream>

namespace rparser {

/*
 * @description
 * chart data contains: notedata, timingdata, metadata
 */
struct ChartSummaryData;
class Chart {
public:
    const MetaData* GetMetaData() const;
    const NoteData* GetNoteData() const;
    const TimingData* GetTimingData() const;
    MetaData* GetMetaData();
    NoteData* GetNoteData();
    TimingData* GetTimingData();
    const ChartSummaryData* GetChartSummary() const;  // cannot modify, read-only
    std::string GetFilePath() const;
    std::string GetHash() const;
    void UpdateChartSummary();
    void UpdateHash(const void* p, int iLen);

    // @description clone myself to another chart
    Chart* Clone();

    // @description change resolution for chart globally
    void ChangeResolution(int newRes);
    // @description call this function when STOP/BPM metadata or object is changed
    // Calling this function will clear all Timingdata, so be careful.
    void UpdateTimingData();
    // @description call this function when calculate fBeat from iRow
    // (cf: playing after editing)
    void UpdateBeatData();

    std::string toString();

    Chart();
    Chart(Chart* c);
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

    int iNoteCount;
    int iTrackCount;
    float fLastNoteTime;    // msec

    // @description basc BPM of the song
    int iBPM;
    int iMaxBPM;
    int iMinBPM;
    bool isBPMChanges;
    // @description is bomb object exists?
    bool isBSS;         // backspin scratch
    bool isCharge;
    bool isHellCharge;
    bool isBomb;
    // @description timingdata related
    bool isWarp;
    bool isStop;
    // @description is command exists/processed? (in case of BMS)
    bool isCommand;

    std::string toString();
};


}

#endif