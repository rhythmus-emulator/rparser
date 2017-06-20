#include "MetaData.h"
#include "util.h"
#include <stdlib.h>

namespace rparser {

template<>
int GetAttribute<int>(const std::string& key, int fallback) const
{
    auto it = m_sAttributes.find(key);
    if (it == m_sAttributes.end()) return fallback;
    const char* s_ptr = it->second.c_str();
    char *endptr;
    int r = strtol(s_ptr, &endptr, 10);
    if (*endptr != 0) return fallback;
    else return r;
}
template<>
double GetAttribute<double>(const std::string& key, double fallback) const
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
int GetAttribute<int>(const std::string& key) const
{
    return GetAttribute<int>(key,0);
}
template<>
double GetAttribute<double>(const std::string& key) const
{
    return GetAttribute<double>(key,0);
}
void MetaData::SetAttribute(const std::string& key, int value)
{
    char s[100];
    itoa(s, 10, value);
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
    gcvt(s, 10, value);
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
        AttemptEncoding(bga.fn, to_codepage, from_codepage);
    }
    for (auto &fn: m_SoundChannel.fn)
    {
        AttemptEncoding(fn, to_codepage, from_codepage);
    }
    for (auto &pair: m_sAttributes)
    {
        AttemptEncoding(pair->second, to_codepage, from_codepage);
    }
}

}