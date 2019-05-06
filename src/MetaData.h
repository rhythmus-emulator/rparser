/*
 * by @lazykuna, MIT License.
 *
 * in this file, we don't care about row position or similar thing.
 * (All placeable objects are located in NoteData.h)
 */

#ifndef RPARSER_METADATA_H
#define RPARSER_METADATA_H

#include "Note.h"
#include "common.h"

namespace rparser {

#define RPARSER_METADATA_LISTS \
  META_STR(title, "TITLE"); \
  META_STR(subtitle, "SUBTITLE"); \
  META_STR(artist, "ARTIST"); \
  META_STR(subartist, "SUBARTIST"); \
  META_STR(genre, "GENRE"); \
  META_STR(charttype, "CHARTTYPE"); \
  META_STR(chartmaker, "CHARTMAKER"); \
  META_INT(player_count, "PLAYERCOUNT"); \
  META_INT(player_side, "PLAYERSIDE"); \
  META_INT(difficulty, "DIFFICULTY"); \
  META_INT(level, "LEVEL"); \
  META_DBL(bpm, "BPM"); \
  META_DBL(judge_timing, "JUDGETIMING"); \
  META_DBL(gauge_total, "TOTAL"); \
  META_DBL(time_0beat_offset, "OFFSET"); \
  META_STR(back_image, "BACKIMAGE"); \
  META_STR(stage_image, "STAGEIMAGE"); \
  META_STR(banner_image, "BANNERIMAGE"); \
  META_STR(preview_music, "PREVIEW"); \
  META_STR(background_music, "BGM"); \
  META_STR(lyrics, "LYRICS"); \
  META_INT(bms_longnote_type, "LNTYPE"); \
  META_INT(bms_longnote_object, "LNOBJ");
  
const static int kDefaultBpm = 120;

struct SoundMetaData {
  // channelno, filename
  std::map<Channel, std::string> fn;
};

struct BgaMetaData {
  struct BgaMetaInfo {
    std::string fn;
    int sx,sy,sw,sh;
    int dx,dy,dw,dh;
  };
  // channelno, bgainfo(filename)
  std::map<Channel, BgaMetaInfo> bga;
};

struct BmsBpmMetaData {
  std::map<Channel, float> bpm;	  // #BPM command (key, value)
  bool GetBpm(Channel channel, float &out) const;
};

struct BmsStopMetaData {
  std::map<Channel, float> stop;  // #STOP command (key, value)
  std::map<float, float> STP;	    // #STP command (time, value)
  bool GetStop(Channel channel, float &out) const;
};

/*
 * @detail
 * contains metadata(header) part for song
 * 
 * @params
 * GetAttribute       get attribute from key
 * IsAttributeExist   check out is attribute is existing
 * SetEncoding        convert encoding of metadata if necessary.
 * SetUtf8Encoding    set metadata into utf8 encoding.
 * DetectEncoding     detect encoding from metadata.
 * 
 * genre
 * charttype          NORMAL, HYPER, ANOTHER, ONI, ...
 * player_count       player count
 * player_side        player side (depreciated, just for BMS format)
 * difficulty         Chart Difficulty (Used on BMS, etc...)
 * level              level of the song
 * bpm                basic BPM (need to be filled automatically if not exists)
 * judge_timing       judge difficulty
 * gauge_total        gauge total
 * time_0beat_offset   start timing offset of 0 beat
 * back_image         BG during playing
 * stage_image        loading image
 * banner_image
 * preview_music
 * background_music   BGM
 * lyrics
 * bms_longnote_type    used for BMS (longnote type)
 * bms_longnote_object  used for BMS
 * encoding           (internal use) encoding of current metadata.
 */
class MetaData {
public:
  MetaData();
  MetaData(const MetaData& m);

  // @description supports int, string, float, double types.
  template<typename T>
  T GetAttribute(const std::string& key) const;
  int GetAttribute(const std::string& key, int fallback=-1) const;
  double GetAttribute(const std::string& key, double fallback=0.0) const;
  void SetAttribute(const std::string& key, int value);
  void SetAttribute(const std::string& key, const std::string& value);
  void SetAttribute(const std::string& key, double value);
  bool IsAttributeExist(const std::string& key);
  void SetMetaFromAttribute();

  bool SetEncoding(int from_codepage, int to_codepage);
  bool SetUtf8Encoding();
  int DetectEncoding();

  // @description
  // It returns valid Channel object pointer always (without exception)
  SoundMetaData* GetSoundChannel() { return &sound_channel_; };
  BgaMetaData* GetBGAChannel() { return &bga_channel_; };
  BmsBpmMetaData* GetBPMChannel() { return &bpm_channel_; };
  BmsStopMetaData* GetSTOPChannel() { return &stop_channel_; };
  const SoundMetaData* GetSoundChannel() const { return &sound_channel_; };
  const BgaMetaData* GetBGAChannel() const { return &bga_channel_; };
  const BmsBpmMetaData* GetBPMChannel() const { return &bpm_channel_; };
  const BmsStopMetaData* GetSTOPChannel() const { return &stop_channel_; };


  #define META_INT(x,s) int x
  #define META_DBL(x,s) double x
  #define META_STR(x,s) std::string x
  RPARSER_METADATA_LISTS
  #undef META_INT
  #undef META_DBL
  #undef META_STR

  int encoding;                   // (internal use) encoding of current metadata.

  std::string toString() const;
  void swap(MetaData &md);
  void clear();
private:
  SoundMetaData sound_channel_;
  BgaMetaData bga_channel_;
  BmsBpmMetaData bpm_channel_;
  BmsStopMetaData stop_channel_;

  // other metadata for some other purpose (key, value)
  std::map<std::string, std::string> attrs_;
};

/**
 * @detail
 * indicates song-specific metadata, which is global and constant.
 */
struct SongMetaData {
  unsigned int track_col_count;
  unsigned int player_max_count;
};

enum class SONGTYPE;

const SongMetaData& GetSongMetaData(SONGTYPE songtype);

}

#endif