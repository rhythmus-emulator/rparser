/*
 * VOS has quite weird format; it contains MIDI file,
 * and it just make user available to play, following MIDI's beat.
 * So, All tempo change/beat/instrument information is contained at MIDI file.
 * VOS only contains key & timing data; NO BEAT DATA.
 * That means we need to generate beat data from time data,
 * So we have to crawl BPM(Tempo) data from MIDI. That may be a nasty thing. 
 */

#include "ChartLoader.h"
#include "Chart.h"
#include <stdlib.h>
#define MAX_READ_SIZE 512000

using namespace rutil;

namespace rparser {

// structure used for parsing data
class ParseArg {
  bool autorelease;
public:
  int iLen;
  const char *p_org;
  const char *p;
  ParseArg(const char *p, int iLen, bool autorelease=true)
  : p(p), p_org(p), iLen(iLen), autorelease(autorelease) {}
  ~ParseArg() { if (autorelease) free(const_cast<char*>(p)); }
  void Reset() { p=p_org; }
};

struct VOSHeader {
  int len;
  char desc[256];
};

struct VOSNoteDataV2 {
  unsigned char dummy;
  unsigned int time;
  unsigned char pitch;
  unsigned char dummy2;   // always 00; WAVID?
  unsigned char volume;
  unsigned char isplayable; // Note or BGM?
  unsigned char issoundable;  // soundable; always 1?
  unsigned char islongnote;
  unsigned int duration;
  unsigned char dummy3; // 00|FF, note or BGM?
};

struct VOSNoteDataV3 {
  unsigned int time;
  unsigned short duration;
  short dummy;
  unsigned char fan;
  unsigned char pitch;
  unsigned char volume;
  unsigned char channel; // playable note?
  unsigned char dummy2;
};

unsigned char MIDISIG_BYTE[3][2] = {
  {0xFF, 0x21},
  {0xFF, 0x03},
  {0xFF, 0x2F}
};

enum class MIDISIG: int {
  MIDISIG_DUMMY=0, // output port
  MIDISIG_TRACKNAME=1,
  MIDISIG_END=2,
  MIDISIG_DATA=3 // includes all unknown other signatures
};

#define MIDISIG_TYPES 3
#define MIDISIG_LENGTH 3

int ReadMSInt( const unsigned char **_p ) {
  const unsigned char *p = *_p;
  int r = 0;
  int i = 0;
  while (true) {
    r = (r << 7) | (*(int*)p & 0x00FFFFFF);
    char sign = *p;
    p += 4;
    if (sign == 0) break;
    ASSERT(++i < 4);
  }

  *_p = p;
  return r;
}

MIDISIG GetMidiSignature(const unsigned char **_p, int *delta) {
  const unsigned char *p = *_p;
  *delta = ReadMSInt(&p);
  MIDISIG r = MIDISIG::MIDISIG_DATA;
  for (int i=0; i<MIDISIG_TYPES; i++) {
    if (memcmp(p, MIDISIG_BYTE[i], MIDISIG_LENGTH) == 0) {
      r = (MIDISIG)i;
      break;
    }
  }
  p += MIDISIG_LENGTH;
  *_p = p;
  return r;
}

ChartLoaderVOS::ChartLoaderVOS()
  : p_(0), note_p_(0), midi_p_(0), vos_version_(VOS_UNKNOWN), len_(0) {};

bool ChartLoaderVOS::LoadFromDirectory(ChartListBase& chartlist, Directory& dir)
{
  if (dir.count() <= 0)
    return false;

  bool r = false;
  auto &f = *dir.begin();
  if (!dir.Read(f.d))
    return false;

  Chart *c = chartlist.GetChartData(chartlist.AddNewChart());
  if (!c) return false;

  c->SetFilename(f.d.GetFilename());
  c->SetHash(rutil::md5_str(f.d.GetPtr(), f.d.GetFileSize()));
  r = Load(*c, f.d.p, f.d.len);

  chartlist.CloseChartData();
  if (!r)
  {
    std::cerr << "Failed to read VOS file (may be invalid) : " << f.d.GetFilename() << std::endl;
  }
  return r;
}

bool ChartLoaderVOS::Load( Chart &c, const void* p, int len ) {
  this->chart_ = &c;
  p_ = static_cast<const unsigned char*>(p);
  note_p_ = 0;
  midi_p_ = 0;
  len_ = len;

  if (!ParseVersion()) return false;

  switch (vos_version_)
  {
  case VOS_V2:
    if ( !(ParseMetaDataV2() && ParseNoteDataV2()) ) return false;
    break;
  case VOS_V3:
    if ( !(ParseMetaDataV3() && ParseNoteDataV3()) ) return false;
    break;
  }

  if (!ParseMIDI()) return false;

  return true;
}

bool ChartLoaderVOS::ParseVersion()
{
  vos_version_ = *(int*)p_;
  switch (vos_version_)
  {
  case VOS_V2:
  case VOS_V3:
    return true;
  }
  vos_version_ = VOS_UNKNOWN;
  return false;
}

bool ChartLoaderVOS::ParseMetaDataV2()
{
  const unsigned char* p = p_ + 4;  // skip version
  MetaData& md = chart_->GetMetaData();

  // 4 byte: filename length
  int fnamelen = *(int*)p;
  p += 4;
  p += fnamelen;
  // 4 byte: VOS track file length
  int vosflen = *(int*)p;
  p += 4;
  // parse metadatas [title, songartist, chartmaker, genre, unknown]
  // metadata consisted with frames
  // frame: length(short) + body
  int i = 0;
  char meta_attrs[][32] = { "TITLE", "ARTIST", "SUBARTIST", "GENRE" };
  while (i < 5) {
    short len = *(short*)p;
    p += 2;
    if (len == 0) break;
    char body[256];
    memset(body, 0, sizeof(body));
    memcpy(body, p, len);
    p += len;
    md.SetAttribute(meta_attrs[i], body);
    i += 1;
  }
  // parse second metadatas (26bytes)
  char meta_binary[26];
  memcpy(meta_binary, p, 26);
  // TODO
  p += 26;
  // if flag is VOS009, then seek 1013 bytes
  // else, seek 1017 bytes
  if (memcmp("VOS009", p, 6) == 0) {
    p += 1013;
  } else {
    p += 1017;
  }

  // done. we'll ignore other metadatas.
  note_p_ = p;
  return true;
}

bool ChartLoaderVOS::ParseMetaDataV3()
{
  const unsigned char* p = p_ + 4;  // skip version
  MetaData& md = chart_->GetMetaData();

  int headersize = *(int*)p; p+=4;
  p += 4; // inf; static flag
  p += 12; // unknown null bytes
  int infpos = *(int*)p; p+=4; // end of inf file pos (start of MID file)
  p += 4; // mid; static flag
  p += 12; // unknown null bytes (maybe 64bit long long int?)
  int midpos = *(int*)p; p+=4;
  p += 4; // EOF flag
  p += 12; // unknown null bytes
  // now position is 64(0x40), which means starting pos of header.
  // start to read METADATAS
  char metadatas[][12] = { "TITLE", "ARTIST", "CHARTMAKER", "GENRE" };
  VOSHeader header;
  for (int i=0; i<4; i++) {
    header.len = *(int*)p; p+=4;
    memcpy(header.desc, p, header.len); p+=header.len;
    header.desc[header.len] = 0;
  }

  int genre = *p; p++;
  p++;    // unknown
  int songlength = *(int*)p; p+=4;
  int level = *p; level++; p++;
  p += 4;
  p++;
  p += 1018; // null; unknown

  // done. we'll ignore other metadatas.
  note_p_ = p;
  return true;
}

bool ChartLoaderVOS::ParseNoteDataV2()
{
  ASSERT(note_p_);
  const unsigned char* p = note_p_;

  int cnt_inst = *(int*)p;     p += 4;
  int cnt_chart = *(int*)p;    p += 4;     // mostly 1, means there're only one chart
  // read basic instrument info for note data filling
  int midichannel_per_track[20];
  int playmode = 0;   // SP or DP ...
  int level = 0;
  std::string title;
  for (int i=0; i<cnt_inst; i++) {
    p += 1;
    midichannel_per_track[i] = *(int*)p;
    p += 4;
  }
  for (int i=0; i<cnt_chart; i++) {
    playmode = *p;  // SP=0, DP=1
    p++;
    level = *p; level++;
    p++;
    short len = *(short*)p;
    char buf[256];
    memcpy(buf, p, len);    p += len;
    title = buf;
    // track index?
    p += 4;
  }
  // parse note objects per each channels.
  // (includes BGM/autoplay objects)
  // TODO: make it as structure
  for (int i=0; i<7; i++) {
    int channel = i;
    int notecnt = *(int*)p;
    p += 4;
    for (int j=0; j<notecnt; j++) {
      VOSNoteDataV2 note;
      memcpy(&note, p, sizeof(VOSNoteDataV2));
      p += sizeof(VOSNoteDataV2);
    }
  }

  return true;
}

bool ChartLoaderVOS::ParseNoteDataV3()
{
  ASSERT(note_p_);
  const unsigned char* p = note_p_;
  
  // start to read note data
  while (true) {
    int midiinstrument = *(int*)p; p+=4;
    short cnt = *(short*)p; p+=4;
    if (cnt == 0) break;
    p += 16;
    for (int i=0; i<cnt; i++) {
      VOSNoteDataV3 note;
      memcpy(&note, p, sizeof(VOSNoteDataV3));
      p += sizeof(VOSNoteDataV3);
    }
  }

  midi_p_ = p;
  return true;
}

bool ChartLoaderVOS::ParseMIDI()
{
  ASSERT(midi_p_);

  const unsigned char* p = midi_p_;
  const unsigned char* p_end = p_+len_;

  // MUST start from MThd signature
  if (memcmp(p, "MThd", 4) != 0) {
    printf("[VOSLoader] Invalid MIDI start signature\n");
    return false;
  }
  p+=4;
  p+=6; // len: 00 00 00 06
  short format = *(short*)p; p+=2; // always 1
  ASSERT(format == 1);
  short trackcount = *(short*)p; p+=2;
  short timedivision = *(short*)p; p+=2;

  int trackidx = 0;
  while (p<p_end && memcmp(p, "Mtrk", 4) == 0) {
    p += 4;
    int tracksize = *(int*)p;
    const unsigned char *p_track_end = p+tracksize;

    MIDISIG midisig = MIDISIG::MIDISIG_DUMMY;
    int delta = 0;
    int ticks = 0;  // 480 tick: 1 beat
    while ((midisig = GetMidiSignature(&p, &delta)) != MIDISIG::MIDISIG_END) {
      ticks += delta;
      if (midisig == MIDISIG::MIDISIG_DATA) {
        // TODO: register midi signature with data in timeloader datas.
      }
    }
    p++; // process 0x00 of MIDISIG_END
    ASSERT(p < p_track_end);
    trackidx ++;
  }

  return true;
}

} /* end rparser */