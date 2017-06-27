#include "MetaData.h"
#include "Chart.h"  // chartsummarydata
#include "util.h"
#include <stdlib.h>

namespace rparser {

// Channels

bool BPMChannel::GetBpm(int channel, float &out) const
{
    auto it = bpm.find(channel);
    if (it == bpm.end()) return false;
    else out = it->second; return true;
}

bool STOPChannel::GetStop(int channel, float &out) const
{
    auto it = stop.find(channel);
    if (it == stop.end()) return false;
    else out = it->second; return true;
}



// ------ class MetaData ------

int MetaData::GetAttribute(const std::string& key, int fallback) const
{
    auto it = m_sAttributes.find(key);
    if (it == m_sAttributes.end()) return fallback;
    const char* s_ptr = it->second.c_str();
    char *endptr;
    int r = strtol(s_ptr, &endptr, 10);
    if (*endptr != 0) return fallback;
    else return r;
}
double MetaData::GetAttribute(const std::string& key, double fallback) const
{
    auto it = m_sAttributes.find(key);
    if (it == m_sAttributes.end()) return fallback;
    const char* s_ptr = it->second.c_str();
    char *endptr;
    double r = strtod(s_ptr, &endptr);
    if (*endptr != 0) return fallback;
    else return r;
}
template<>
int MetaData::GetAttribute(const std::string& key) const
{
    return GetAttribute(key,0);
}
template<>
double MetaData::GetAttribute(const std::string& key) const
{
    return GetAttribute(key,.0);
}
void MetaData::SetAttribute(const std::string& key, int value)
{
    char s[100];
    itoa(value, s, 10);
    SetAttribute(key ,s);
}
void MetaData::SetAttribute(const std::string& key, const std::string& value)
{
    m_sAttributes[key] = value;
}
void MetaData::SetAttribute(const std::string& key, double value)
{
    // enough space to assign string
    char s[100];
    gcvt(value, 10, s);
    SetAttribute(key, s);
}
bool MetaData::IsAttributeExist(const std::string& key)
{
    return m_sAttributes.find(key) != m_sAttributes.end();
}
bool MetaData::SetEncoding(int from_codepage, int to_codepage)
{
    AttemptEncoding(sTitle, to_codepage, from_codepage);
    AttemptEncoding(sSubTitle, to_codepage, from_codepage);
    AttemptEncoding(sArtist, to_codepage, from_codepage);
    AttemptEncoding(sSubArtist, to_codepage, from_codepage);
    AttemptEncoding(sGenre, to_codepage, from_codepage);
    AttemptEncoding(sChartName, to_codepage, from_codepage);
    AttemptEncoding(sBackImage, to_codepage, from_codepage);
    AttemptEncoding(sStageImage, to_codepage, from_codepage);
    AttemptEncoding(sBannerImage, to_codepage, from_codepage);
    AttemptEncoding(sPreviewMusic, to_codepage, from_codepage);
    AttemptEncoding(sMusic, to_codepage, from_codepage);
    AttemptEncoding(sLyrics, to_codepage, from_codepage);
    AttemptEncoding(sExpand, to_codepage, from_codepage);

    // also do encoding process for all Sound/BGA channels
    for (auto &bga: m_BGAChannel.bga)
    {
        AttemptEncoding(bga.second.fn, to_codepage, from_codepage);
    }
    for (auto &fn: m_SoundChannel.fn)
    {
        AttemptEncoding(fn.second, to_codepage, from_codepage);
    }
    for (auto &pair: m_sAttributes)
    {
        AttemptEncoding(pair.second, to_codepage, from_codepage);
    }
}
void MetaData::FillSummaryData(ChartSummaryData &csd) const
{
    csd.isCommand = !sExpand.empty();
    csd.sTitle = sTitle;
    csd.sSubTitle = sSubTitle;
    csd.sLongTitle = sTitle + " " + sSubTitle;
    csd.sArtist = sArtist;
    csd.sGenre = sGenre;
}

std::string MetaData::toString()
{
    std::stringstream ss;
    ss << "Title: " << sTitle << std::endl;
    ss << "SubTitle: " << sSubTitle << std::endl;
    ss << "Artist: " << sArtist << std::endl;
    ss << "Genre: " << sGenre << std::endl;
    ss << "Is Expand Command? : " << !sExpand.empty() << std::endl;
    return ss.str();
}

}