#include "MetaData.h"
#include "Chart.h"  // chartsummarydata
#include "rutil.h"
#include <stdlib.h>

using namespace rutil;

#define RPARSER_METADATA_LISTS \
  META_STR(title); \
  META_STR(subtitle); \
  META_STR(artist); \
  META_STR(subartist); \
  META_STR(genre); \
  META_STR(charttype); \
  META_INT(player); \
  META_INT(difficulty); \
  META_INT(level); \
  META_DBL(bpm); \
  META_DBL(judge_timing); \
  META_DBL(gauge_total); \
  META_STR(back_image); \
  META_STR(stage_image); \
  META_STR(banner_image); \
  META_STR(preview_music); \
  META_STR(background_music); \
  META_STR(lyrics); \
  META_INT(bms_longnote_type); \
  META_INT(bms_longnote_object);

namespace rparser {

bool BmsBpmMetaData::GetBpm(Channel channel, float &out) const
{
  auto it = bpm.find(channel);
  if (it == bpm.end()) return false;
  else out = it->second; return true;
}

bool BmsStopMetaData::GetStop(Channel channel, float &out) const
{
  auto it = stop.find(channel);
  if (it == stop.end()) return false;
  else out = it->second; return true;
}


MetaData::MetaData()
{
  clear();
}

MetaData::MetaData(const MetaData& m)
{
  #define META_INT(x) x = m.x;
  #define META_DBL(x) x = m.x;
  #define META_STR(x) x = m.x;
  RPARSER_METADATA_LISTS
  #undef META_INT
  #undef META_DBL
  #undef META_STR

  encoding = m.encoding;
}

int MetaData::GetAttribute(const std::string& key, int fallback) const
{
  auto it = attrs_.find(key);
  if (it == attrs_.end()) return fallback;
  const char* s_ptr = it->second.c_str();
  char *endptr;
  int r = strtol(s_ptr, &endptr, 10);
  if (*endptr != 0) return fallback;
  else return r;
}

double MetaData::GetAttribute(const std::string& key, double fallback) const
{
  auto it = attrs_.find(key);
  if (it == attrs_.end()) return fallback;
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
  _itoa(value, s, 10);
  SetAttribute(key ,s);
}

void MetaData::SetAttribute(const std::string& key, const std::string& value)
{
  attrs_[key] = value;
}

void MetaData::SetAttribute(const std::string& key, double value)
{
  // XXX: enough space to assign string?
  char s[100];
  _gcvt(value, 10, s);
  SetAttribute(key, s);
}

bool MetaData::IsAttributeExist(const std::string& key)
{
  return attrs_.find(key) != attrs_.end();
}

bool MetaData::SetEncoding(int from_codepage, int to_codepage)
{
  #define META_INT(x)
  #define META_DBL(x)
  #define META_STR(x) AttemptEncoding(x, to_codepage, from_codepage)
  RPARSER_METADATA_LISTS
  #undef META_INT
  #undef META_DBL
  #undef META_STR

  // also do encoding process for all Sound/BGA channels
  for (auto &bga: bga_channel_.bga)
  {
    AttemptEncoding(bga.second.fn, to_codepage, from_codepage);
  }
  for (auto &fn: sound_channel_.fn)
  {
    AttemptEncoding(fn.second, to_codepage, from_codepage);
  }
  for (auto &pair: attrs_)
  {
    AttemptEncoding(pair.second, to_codepage, from_codepage);
  }

  return true;
}

bool MetaData::SetUtf8Encoding()
{

}

int MetaData::DetectEncoding()
{

}

std::string MetaData::toString()
{
    std::stringstream ss;
    ss << "Title: " << title << std::endl;
    ss << "SubTitle: " << subtitle << std::endl;
    ss << "Artist: " << artist << std::endl;
    ss << "Genre: " << genre << std::endl;
    ss << "ChartType: " << charttype << std::endl;
    ss << "Difficulty: " << difficulty << std::endl;
    ss << "Encoding: " << encoding << std::endl;
    return ss.str();
}

void MetaData::swap(MetaData& m)
{
  #define META_INT(x) std::swap(x, m.x)
  #define META_DBL(x) std::swap(x, m.x)
  #define META_STR(x) std::swap(x, m.x)
  RPARSER_METADATA_LISTS
  #undef META_INT
  #undef META_DBL
  #undef META_STR
}

void MetaData::clear()
{
  #define META_INT(x) x = 0;
  #define META_DBL(x) x = 0.0;
  #define META_STR(x)
  RPARSER_METADATA_LISTS
  #undef META_INT
  #undef META_DBL
  #undef META_STR

  bpm = kDefaultBpm;
  player = 1;
}

}