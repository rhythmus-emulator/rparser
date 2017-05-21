#include "TimingData.h"

namespace rparser {

BpmObject* TimingData::GetNextBpmObject(int iStartRow)
{
    return 0;
}

StopObject* TimingData::GetNextStopObject(int iStartRow)
{
    return 0;
}

WarpObject* TimingData::GetNextWarpObject(int iStartRow)
{
    return 0;
}

MeasureObject* TimingData::GetNextMeasureObject(int iStartRow)
{
    return 0;
}

void TimingData::GetBpm()
{
    // first part BPM, mainly used for metadata
    return 0;
}

// measure related
void TimingData::SetBarLengthAtRow(int row, float length=4.0f)
{
    return 0;
}

float TimingData::GetBarLengthAtRow(int row)
{
    return 0;
}

float TimingData::GetBarBeat(int barnumber)
{
    return 0;
}

// functions using sequential objects
float TimingData::GetBeatFromTime(float time)
{
    return 0;
}

float TimingData::GetTimeFromBeat(float beat)
{
    return 0;
}

int TimingData::GetNextMeasureFromTime(float timeoffset)
{
    return 0;
}

// @description
// MUST be called before we use Sequential objects & midi events.
// (use after all BPM/STOP/WARP/MEASURE objects are loaded)
void TimingData::Invalidate()
{
    return 0;
}

}