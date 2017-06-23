#include "Chart.h"
#include "TimingData.h"
#include <sstream>
#include <openssl/md5.h>

namespace rparser
{

Chart::Chart()
{
    m_ChartSummaryData.iBPM = 120;
    m_ChartSummaryData.iMaxBPM = 120;
    m_ChartSummaryData.iMinBPM = 120;
    m_ChartSummaryData.iNoteCount = 0;
    m_ChartSummaryData.iTrackCount = 0;
    m_ChartSummaryData.fLastNoteTime = 0;
    m_ChartSummaryData.isBomb = false;
    m_ChartSummaryData.isBPMChanges = false;
    m_ChartSummaryData.isBSS = false;
    m_ChartSummaryData.isCharge = false;
    m_ChartSummaryData.isCommand = false;
    m_ChartSummaryData.isHellCharge = false;
    m_ChartSummaryData.isStop = false;
    m_ChartSummaryData.isWarp = false;
}

Chart::Chart(Chart* c)
{
    m_ChartSummaryData = c->m_ChartSummaryData;
    m_Metadata = c->m_Metadata;
    m_Notedata = c->m_Notedata;
    m_Timingdata = c->m_Timingdata;
}

const MetaData* Chart::GetMetaData() const
{
    return &m_Metadata;
}
const NoteData* Chart::GetNoteData() const
{
    return &m_Notedata;
}
const TimingData* Chart::GetTimingData() const
{
    return &m_Timingdata;
}
MetaData* Chart::GetMetaData()
{
    return &m_Metadata;
}
NoteData* Chart::GetNoteData()
{
    return &m_Notedata;
}
TimingData* Chart::GetTimingData()
{
    return &m_Timingdata;
}
const ChartSummaryData* Chart::GetChartSummary() const
{
    return &m_ChartSummaryData;
}
std::string Chart::GetFilePath() const
{
    return m_ChartSummaryData.sFilePath;
}
std::string Chart::GetHash() const
{
    return m_ChartSummaryData.sHash;
}
void Chart::UpdateHash(const void* p, int iLen)
{
    unsigned char result[MD5_DIGEST_LENGTH];
    MD5((unsigned char*)p, iLen, result);
    std::string hash_str;
    char buf[6];
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
    {
        sprintf(buf, "%02x", result[i]);
        hash_str.append(buf);
    }
    m_ChartSummaryData.sHash = hash_str;
}

std::string Chart::toString()
{
    //std::stringstream ss;
    //ss << m_ChartSummaryData.toString() << std::endl;
    //ss << m_Metadata.toString() << std::endl;
    //ss << m_Notedata.toString() << std::endl;
    //ss << m_Timingdata.toString() << std::endl;
    //return ss.str();
    return m_ChartSummaryData.toString();
}



void Chart::UpdateChartSummary()
{
    m_Notedata.FillSummaryData(m_ChartSummaryData);
    // fill metadata after notedata, as Trackcount should be overwritten.


    m_ChartSummaryData.fLastNoteTime = m_Notedata.GetLastNoteTime();
    // iterate through Note objects
    m_ChartSummaryData.isBomb = false;
    m_ChartSummaryData.isBSS = false;
    m_ChartSummaryData.isCharge = false;
    for (auto it = m_Notedata.begin(); it != m_Notedata.end(); ++it)
    {
        if (it->IsTappableNote())
        {
            m_ChartSummaryData.iNoteCount++;
        }
        m_ChartSummaryData.isBomb |= it->nType == NoteTapType::TAPNOTE_MINE;
        m_ChartSummaryData.isCharge |= it->nType == NoteTapType::TAPNOTE_CHARGE;
        if (it->GetTrack() % 10 == 0)
            m_ChartSummaryData.isBSS |= it->nType == NoteTapType::TAPNOTE_CHARGE;
    }
    // iterate through Timing objects
    m_ChartSummaryData.isWarp = m_Timingdata.HasWarp();
    m_ChartSummaryData.isStop = m_Timingdata.HasStop();
    m_ChartSummaryData.isBPMChanges = m_Timingdata.HasBpmChange();
    // iscommand
    m_ChartSummaryData.isCommand = !m_Metadata.sExpand.empty();
    // TODO hellcharge from metadata
}
Chart* Chart::Clone()
{
    return new Chart(this);
}

void Chart::ChangeResolution(int newRes)
{
    m_Timingdata.SetResolution(newRes);
    m_Notedata.SetResolution(newRes);
}

void Chart::UpdateTimingData()
{
    // invalidate timingdata from meta/notedata
    m_Timingdata.ClearExternObject();
    m_Timingdata.LoadExternObject(m_Metadata, m_Notedata);
}

void Chart::UpdateBeatData()
{
    m_Timingdata.UpdateBeatData();
    m_Notedata.UpdateBeatData();
}


// chart summary
std::string ChartSummaryData::toString()
{
    std::stringstream ss;
    ss << "Chart information" << std::endl;
    ss << "Path: " << sFilePath << std::endl;
    ss << "File format: " << sFormat << std::endl;
    ss << "File MD5 Hash: " << sHash << std::endl;
    ss << "Note Count: " << iNoteCount << std::endl;
    ss << "Track Count: " << iTrackCount << std::endl;
    ss << "Last note time: " << fLastNoteTime << std::endl;
    ss << "BPM: " << iBPM << std::endl;
    ss << "MaxBPM: " << iMaxBPM << std::endl;
    ss << "MinBPM: " << iMinBPM << std::endl;
    ss << "isBPMChanges: " << isBPMChanges << std::endl;
    ss << "isBomb: " << isBomb << std::endl;
    ss << "isWarp: " << isWarp << std::endl;
    ss << "isStop: " << isStop << std::endl;
    ss << "is command exists?: " << isCommand << std::endl;
    return ss.str();
}

}