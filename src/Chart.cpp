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
    m_NoteData->ApplyResolutionRatio(fRatio);
}