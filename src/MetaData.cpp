#include "MetaData.h"
#include "Song.h"   // enum class SONGTYPE
#include "rutil.h"
#include <stdlib.h>

using namespace rutil;

namespace rparser {

bool BmsBpmMetaData::GetBpm(Channel channel, float &out) const
{
  auto it = bpm.find(channel);
  if (it == bpm.end()) return false;
  else out = static_cast<float>(it->second); return true;
}

bool BmsStopMetaData::GetStop(Channel channel, float &out) const
{
  auto it = stop.find(channel);
  if (it == stop.end()) return false;
  else out = static_cast<float>(it->second); return true;
}


MetaData::MetaData()
{
  clear();
}

MetaData::MetaData(const MetaData& m)
{
  #define META_INT(x,s) x = m.x;
  #define META_DBL(x,s) x = m.x;
  #define META_STR(x,s) x = m.x;
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
  rutil::itoa(value, s, 10);
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
  rutil::gcvt(value, 10, s);
  SetAttribute(key, s);
}

void MetaData::MergeAttributes(const MetaData& md)
{
  for (auto ii : md.attrs_)
  {
    SetAttribute(ii.first, ii.second);
  }
}

bool MetaData::IsAttributeExist(const std::string& key)
{
  return attrs_.find(key) != attrs_.end();
}

void MetaData::SetMetaFromAttribute()
{
  for (const auto &attr : attrs_)
  {
    if (attr.first.size() == 0) continue;
#define META_INT(x,s) else if (attr.first == s) x = atoi(attr.second.c_str())
#define META_DBL(x,s) else if (attr.first == s) x = atof(attr.second.c_str())
#define META_STR(x,s) else if (attr.first == s) x = attr.second
    RPARSER_METADATA_LISTS
#undef META_STR
#undef META_DBL
#undef META_INT
    /** BMS related attributes */
    if (attr.first == "DEFEXRANK") {
      // convert from 160 to 100.0
      judgerank = atof(attr.second.c_str()) / 160;
    }
    else if (attr.first == "RANK") {
      // convert from 4 to 100.0
      judgerank = atof(attr.second.c_str()) / 4.0 * 100;
    }
    else if (attr.first == "PLAYER") {
      int pl = atoi(attr.second.c_str());
      if (pl == 2 || pl == 4)
        player_count = 2;
      else
        player_count = 1;
      player_side = 1;
    }
  }
}

bool MetaData::SetEncoding(int from_codepage, int to_codepage)
{
  #define META_INT(x,s)
  #define META_DBL(x,s)
  #define META_STR(x,s) x = std::move(ConvertEncoding(x, to_codepage, from_codepage))
  RPARSER_METADATA_LISTS
  #undef META_INT
  #undef META_DBL
  #undef META_STR

  // also do encoding process for all Sound/BGA channels
  for (auto &bga: bga_channel_.bga)
  {
    bga.second.fn = std::move(ConvertEncoding(bga.second.fn, to_codepage, from_codepage));
  }
  for (auto &fn: sound_channel_.fn)
  {
    fn.second = std::move(ConvertEncoding(fn.second, to_codepage, from_codepage));
  }
  for (auto &pair: attrs_)
  {
    pair.second = std::move(ConvertEncoding(pair.second, to_codepage, from_codepage));
  }

  return true;
}

bool MetaData::SetUtf8Encoding()
{
  if (DetectEncoding() != E_UTF8)
    return SetEncoding(encoding, E_UTF8);
  else return true;
}

int MetaData::DetectEncoding()
{
  // XXX: just try UTF-8, Shift-JIS, EUC-KR (otherwise).
  std::stringstream encoding_test_ss;
  encoding_test_ss << title << subtitle << artist << genre;
  encoding = rutil::DetectEncoding(encoding_test_ss.str());
  return encoding;
}

std::string MetaData::toString() const
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
  #define META_INT(x,s) std::swap(x, m.x)
  #define META_DBL(x,s) std::swap(x, m.x)
  #define META_STR(x,s) std::swap(x, m.x)
  RPARSER_METADATA_LISTS
  #undef META_INT
  #undef META_DBL
  #undef META_STR
}

void MetaData::clear()
{
  #define META_INT(x,s) x = 0;
  #define META_DBL(x,s) x = 0.0;
  #define META_STR(x,s)
  RPARSER_METADATA_LISTS
  #undef META_INT
  #undef META_DBL
  #undef META_STR

  script.clear();
  bpm = kDefaultBpm;
  player_count = 1;
}

}
