#include "Chart.h"


void Chart::UpdateChartSummary()
{
    m_ChartSummaryData.Fill(this);
    // TODO: fill md5 hash data if available file
}

void Chart::ChangeResolution(int newRes)
{
    float fRatio = (float)newRes / m_Timingdata.GetResolution();
    m_Timingdata.SetResolution(newRes);
    m_NoteData.SetResolution(newRes);
}

void Chart::UpdateTimingData()
{
    m_Timingdata.LoadFromMetaData(m_Metadata);
    m_Timingdata.LoadFromNoteData(m_Notedata);
}

void Chart::UpdateBeatData()
{
    m_Notedata.UpdateBeatData(m_Timingdata.GetResolution());
    m_Timingdata.UpdateBeatData(m_Timingdata.GetResolution());
}