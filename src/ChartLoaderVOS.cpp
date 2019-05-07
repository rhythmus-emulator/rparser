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
  uint8_t dummy;
  uint32_t time;
  uint16_t pitch;
  uint8_t volume;
  uint8_t isplayable; // Note or BGM?
  uint8_t issoundable;  // soundable; always 1?
  uint8_t islongnote;
  uint32_t duration;
  uint8_t dummy3;
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

#define MIDISIG_TYPES 8
#define MIDISIG_LENGTH 2
#define MIDISIG_NULLCMPR 0xFF /* this byte wont be compared */

unsigned char MIDISIG_BYTE[MIDISIG_TYPES][MIDISIG_LENGTH] = {
  {0xFF, 0x21},
  {0xFF, 0x03},
  {0xFF, 0x2F},
  {0xFF, 0x51},
  {0xFF, 0x58},
  {0xFF, 0x59},
  {0xFF, 0x20},
  {0xFF, 0x00},
};

// https://www.csie.ntu.edu.tw/~r92092/ref/midi/#sysex_event
enum class MIDISIG: int {
  MIDISIG_DUMMY=0, // output port
  MIDISIG_TRACKNAME,
  MIDISIG_END,
  MIDISIG_TEMPO,
  MIDISIG_MEASURE,
  MIDISIG_KEYSIG,
  MIDISIG_CHANNEL,
  MIDISIG_SEQNUM,
  // -- below is for program --
  MIDISIG_PROGRAM,
  MIDISIG_PROGRAM_RUNNING,
  MIDISIG_OTHERS
};

ChartLoaderVOS::BinaryStream::BinaryStream()
  : p_(0), len_(0), offset_(0) {}

void ChartLoaderVOS::BinaryStream::SetSource(const void* p, size_t len)
{
  p_ = p;
  len_ = len;
  offset_ = 0;
}

void ChartLoaderVOS::BinaryStream::SeekSet(size_t cnt)
{
  offset_ = cnt;
  ASSERT(offset_ < len_);
}

void ChartLoaderVOS::BinaryStream::SeekCur(size_t cnt)
{
  offset_ += cnt;
  ASSERT(offset_ < len_);
}

int32_t ChartLoaderVOS::BinaryStream::ReadInt32()
{
  return *reinterpret_cast<const int32_t*>((uint8_t*)p_ + offset_);
}

uint8_t ChartLoaderVOS::BinaryStream::ReadUInt8()
{
  return *reinterpret_cast<const uint8_t*>((uint8_t*)p_ + offset_);
}

int32_t ChartLoaderVOS::BinaryStream::GetInt32()
{
  int32_t r = *reinterpret_cast<const int32_t*>((uint8_t*)p_ + offset_);
  offset_ += sizeof(int32_t);
  ASSERT(offset_ < len_);
  return r;
}

uint32_t ChartLoaderVOS::BinaryStream::GetUInt32()
{
  uint32_t r = *reinterpret_cast<const uint32_t*>((uint8_t*)p_ + offset_);
  offset_ += sizeof(uint32_t);
  ASSERT(offset_ < len_);
  return r;
}

uint16_t ChartLoaderVOS::BinaryStream::GetUInt16()
{
  uint16_t r = *reinterpret_cast<const uint16_t*>((uint8_t*)p_ + offset_);
  offset_ += sizeof(uint16_t);
  ASSERT(offset_ < len_);
  return r;
}

uint8_t ChartLoaderVOS::BinaryStream::GetUInt8()
{
  uint8_t r = *reinterpret_cast<const uint8_t*>((uint8_t*)p_ + offset_);
  offset_ += sizeof(uint8_t);
  ASSERT(offset_ < len_);
  return r;
}

void ChartLoaderVOS::BinaryStream::GetChar(char *out, size_t cnt)
{
  memcpy(out, (uint8_t*)p_ + offset_, cnt);
  offset_ += cnt;
  ASSERT(offset_ < len_);
}

int ChartLoaderVOS::BinaryStream::GetOffset()
{
  return offset_;
}

int ChartLoaderVOS::BinaryStream::GetMSFixedInt(uint8_t bytesize)
{
  int r = 0;
  for (int i = 0; i < bytesize; i++)
  {
    r = (r << 8) + *((uint8_t*)p_ + offset_);
    offset_++;
  }
  return r;
}

int ChartLoaderVOS::BinaryStream::GetMSInt()
{
  int r = 0;
  bool cont_flag = false;
  do
  {
    cont_flag = *((uint8_t*)p_ + offset_) & 0x80;
    r = (r << 7) + (*((uint8_t*)p_ + offset_) & 0x7F);
    offset_++;
  } while (cont_flag);
  return r;
}

MIDISIG ChartLoaderVOS::BinaryStream::GetMidiSignature(MIDIProgramInfo &mprog)
{
  uint8_t buf[20];
  GetChar((char*)buf, MIDISIG_LENGTH);
  MIDISIG r = MIDISIG::MIDISIG_OTHERS;
  if (buf[0] < 0xF0)
  {
    /** Channel Message */
    r = MIDISIG::MIDISIG_PROGRAM;
    uint8_t *st_buf = buf + 1;
    uint8_t st_len = 2;

    if (buf[0] <= 0x80)
    {
      /** MUST BE RUNNING STATUS */
      ASSERT(mprog.cmdtype != 0);
      st_buf = buf;
    }
    else
    {
      mprog.cmdtype = buf[0];
      buf[2] = GetUInt8();
    }

    if ((0xC0 < mprog.cmdtype) && (mprog.cmdtype < 0xE0))
    {
      st_len = 1;
      offset_--;
    }

    mprog.cmd[0] = st_buf[0];
    mprog.cmd[1] = (st_len == 1) ? 0 : st_buf[1];
  }
  else
  {
    /** Event Message */
    mprog.cmdtype = 0;  // clear running status flag

    if (buf[0] == 0xF0 || buf[0] == 0xF7)
    {
      std::cerr << "MIDI Sysex event found, going to be ignored." << std::endl;
      return r;
    }

    for (int i = 0; i < MIDISIG_TYPES; i++) {
      if (memcmp(buf, MIDISIG_BYTE[i], MIDISIG_LENGTH) == 0)
      {
        r = (MIDISIG)i;
        break;
      }
    }
  }

  return r;
}

bool ChartLoaderVOS::BinaryStream::IsEnd()
{
  return offset_ >= len_;
}

ChartLoaderVOS::ChartLoaderVOS()
  : vos_version_(VOS_UNKNOWN) {};

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
  stream.SetSource(p, len);

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
  vos_version_ = stream.ReadInt32();
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
  stream.GetUInt32();     // skip version
  MetaData& md = chart_->GetMetaData();
  char buf[256];

  // 4 byte: filename length
  int fnamelen = stream.GetInt32();
  stream.SeekCur(fnamelen);
  // 4 byte: VOS track file length
  int vosflen = stream.GetInt32();
  // 6 byte: signature
  stream.GetChar(buf, 6);
#if 0
  ASSERT(memcmp(buf, "VOS022", 6) == 0);
#else
  ASSERT(memcmp(buf, "VOS", 3) == 0);
#endif
  // parse metadatas [title, songartist, chartmaker, genre, unknown]
  // metadata consisted with frames
  // frame: length(short) + body
  int i = 0;
  char meta_attrs[][32] = { "TITLE", "ARTIST", "SUBARTIST", "CHARTMAKER", "GENRE" };
  while (i < 5) {
    uint16_t len = stream.GetUInt16();
    if (len == 0) break;
    memset(buf, 0, sizeof(buf));
    stream.GetChar(buf, len);
    md.SetAttribute(meta_attrs[i], buf);
    i += 1;
  }
  md.SetMetaFromAttribute();

  // parse second metadatas (28bytes)
  // TODO
  char meta_binary[28];
  stream.GetChar(meta_binary, 28);
#if 0
  // if flag is VOS009, then seek 1013 bytes
  // else, seek 1017 bytes
  if (memcmp("VOS009", p, 6) == 0) {
    p += 1013;
  } else {
    p += 1017;
  }
#endif

  stream.SeekCur(0x3F7);

  // done. we'll ignore other metadatas.
  return true;
}

bool ChartLoaderVOS::ParseMetaDataV3()
{
  char buf[256];
  stream.SeekSet(4);  // skip version
  MetaData& md = chart_->GetMetaData();

  int headersize = stream.GetInt32();
  stream.SeekCur(4);  // inf; static flag
  stream.SeekCur(12); // unknown null bytes
  vos_v3_midi_offset_ = stream.GetInt32(); // end of inf file pos (start of MID file)
  stream.SeekCur(4);  // mid; static flag
  stream.SeekCur(12); // unknown null bytes (maybe 64bit long long int?)
  int midpos = stream.GetInt32();
  stream.SeekCur(4);  // EOF flag
  stream.GetChar(buf, 12);  // some unknown bytes
  // some vos file header is different ...
  const char **metadatas = 0;
  int metadata_cnt = 0;
  if (buf[0] == 0)
  {
    stream.GetChar(buf, 0x46);
    ASSERT(memcmp(buf, "VOS1", 4) == 0);
    const char *metadatas1[] = { "ARTIST", "TITLE", "SUBARTIST", "GENRE", "CHARTMAKER" };
    metadatas = metadatas1;
    metadata_cnt = 5;
  }
  else
  {
    const char *metadatas1[] = { "ARTIST", "TITLE", "SUBARTIST", "CHARTMAKER" };
    metadatas = metadatas1;
    metadata_cnt = 4;
  }
  // now position is 64(0x40), which means starting pos of header.
  // start to read METADATAS
  VOSHeader header;
  for (int i=0; i<metadata_cnt; i++) {
    header.len = stream.GetUInt8();
    stream.GetChar(header.desc, header.len);
    header.desc[header.len] = 0;
    md.SetAttribute(metadatas[i], header.desc);
  }
  md.SetMetaFromAttribute();

  int genre = stream.GetUInt8();
  stream.GetUInt8(); /* unknown 1 byte? */
  int songlength = stream.GetUInt32();
  int level = stream.GetUInt8(); level++;
  stream.SeekCur(4 + 1 + 1018);

  // done. we'll ignore other metadatas.
  return true;
}

bool ChartLoaderVOS::ParseNoteDataV2()
{
  char buf[256];

  int cnt_inst = stream.GetInt32();
  int cnt_chart = stream.GetInt32();  // mostly 1, means there're only one chart
  // read basic instrument info for note data filling
  int midichannel_per_track[20];
  int playmode = 0;   // SP or DP ...
  int level = 0;
  std::string title;
  for (int i=0; i<cnt_inst; i++) {
    stream.SeekCur(1);
    midichannel_per_track[i] = stream.GetInt32();
  }
  for (int i=0; i<cnt_chart; i++) {
    playmode = stream.GetUInt8();  // SP=0, DP=1
    level = stream.GetUInt8(); level++;
    uint16_t len = stream.GetUInt16();
    stream.GetChar(buf, len);
    title = buf;
    // track index? (int 0)
    stream.SeekCur(4);
  }

  // parse note objects per each channels.
  // (includes BGM/autoplay objects)
  // TODO: make it as structure
  VOSNoteDataV2 vnote;
  SoundNote note;
  auto &nd = chart_->GetNoteData();
  for (int i=0; i<cnt_inst; i++) {
    int channel = i;
    int notecnt = stream.GetInt32();
    for (int j=0; j<notecnt; j++) {
      /** cannot do memcpy due to struct padding. */
      vnote.dummy = stream.GetUInt8();
      vnote.time = stream.GetUInt32();
      vnote.pitch = stream.GetUInt16();
      vnote.volume = stream.GetUInt8();
      vnote.isplayable = stream.GetUInt8();
      vnote.issoundable = stream.GetUInt8();
      vnote.islongnote = stream.GetUInt8();
      vnote.duration = stream.GetUInt32();
      vnote.dummy3 = stream.GetUInt8();

      note.SetTimePos(vnote.time);
      if (vnote.isplayable)
        note.SetAsTapNote(0, i);
      else
        note.SetAsBGM(i);
      nd.AddNote(note);
    }
  }

  /** Actual playable note segment */
  ASSERT(stream.GetUInt32() == 0);        // empty
  uint32_t seg_cnt = stream.GetUInt32();  // some unknown segment count
  stream.SeekCur(1);
  for (size_t i = 0; i < seg_cnt; i++)
  {
    uint32_t idx = stream.GetUInt32();    // idx
    uint8_t lane = stream.GetUInt8();
    uint8_t ln_size = stream.GetUInt8();  // seems like instrument channel?
    //std::cout << idx << "," << (int)lane << " (" << (int)ln_size << ")" << std::endl;
  }
  stream.SeekCur(7);

  return true;
}

bool ChartLoaderVOS::ParseNoteDataV3()
{
  // start to read note data
  VOSNoteDataV3 note;
  uint16_t cnt;

  while (stream.GetOffset() < vos_v3_midi_offset_) {
    int midiinstrument = stream.GetInt32();
    cnt = stream.GetUInt16();
    stream.SeekCur(16);
    for (int i=0; i<cnt; i++) {
      note.time = stream.GetUInt32();
      note.duration = stream.GetUInt16();
      note.dummy = stream.GetUInt16();
      note.fan= stream.GetUInt8();
      note.pitch = stream.GetUInt8();
      note.volume = stream.GetUInt8();
      note.channel = stream.GetUInt8();
      note.dummy2 = stream.GetUInt8();
      //std::cout << note.time << "," << (int)note.channel << "," << (int)note.fan << std::endl;
    }
  }

  return true;
}

bool ChartLoaderVOS::ParseMIDI()
{
  char buf[256];
  char *buf_midi = (char*)malloc(65536);

  switch (vos_version_)
  {
  case 2:
  {
    // in case of version 2, skip file name.
    uint32_t fnamelen = stream.GetUInt32();
    stream.GetChar(buf, fnamelen);
    stream.GetUInt32(); // filesize
    break;
  }
  case 3:
    ASSERT(stream.GetOffset() == vos_v3_midi_offset_);
    break;
  default:
    ASSERT(0);
  }

  // MUST start from MThd signature
  stream.GetChar(buf, 4);
  if (memcmp(buf, "MThd", 4) != 0) {
    std::cerr << "[VOSLoader] Invalid MIDI start signature" << std::endl;
    return false;
  }
  stream.SeekCur(5);  // len: 00 00 00 06
  uint16_t format = stream.GetUInt16();       // always 1
  ASSERT(format == 1);
  uint16_t trackcount = stream.GetUInt16();
  uint16_t timedivision = stream.GetUInt8();  // tick size
  double cur_beat = 0;

  TempoNote tn;
  SoundNote sn;
  auto &td = chart_->GetTempoData();
  auto &tnd = td.GetTempoNoteData();
  auto &nd = chart_->GetNoteData();
  uint32_t val, val2;

  int trackidx = 0;
  do {
    stream.GetChar(buf, 4);
    if (memcmp(buf, "MTrk", 4) != 0)
      break;

    uint32_t tracksize = stream.GetMSFixedInt();
    uint32_t offset_cur = stream.GetOffset();

    MIDISIG midisig = MIDISIG::MIDISIG_DUMMY;
    MIDIProgramInfo mprog;
    mprog.cmdtype = 0;
    int delta = 0;  // delta tick(position) from previous event.
    int ticks = 0;  // current midi event position (tick). 480 tick: 1 measure (4 beat), mostly.
    int byte_len = 0;
    bool is_midisegment_end = false;
    do {
      delta = stream.GetMSInt();
      midisig = stream.GetMidiSignature(mprog);
      ASSERT(stream.GetOffset() < offset_cur + tracksize);
      ticks += delta;
      cur_beat = (double)ticks / timedivision;

      if (midisig != MIDISIG::MIDISIG_PROGRAM)
        byte_len = stream.GetMSInt();

      switch (midisig)
      {
      case MIDISIG::MIDISIG_PROGRAM:
        // TODO
        break;
      case MIDISIG::MIDISIG_TEMPO:
        ASSERT(byte_len == 3);
        val = stream.GetMSFixedInt(3);
        tn.SetBeatPos(cur_beat);
        tn.SetBpm((double)val / timedivision / 500000 * 120);
        tnd.AddNote(tn);
        break;
      case MIDISIG::MIDISIG_MEASURE:
        val = stream.GetUInt8();
        val2 = stream.GetUInt8();
        stream.GetUInt8();  // useless
        stream.GetUInt8();  // useless
        tn.SetBeatPos(cur_beat);
        tn.SetMeasure(4.0 * val / val2);
        tnd.AddNote(tn);
        break;
      case MIDISIG::MIDISIG_KEYSIG:
        // TODO: special object
        break;
      case MIDISIG::MIDISIG_CHANNEL:
        // TODO: special object
        break;
      case MIDISIG::MIDISIG_SEQNUM:
        // TODO: special object
        break;
      case MIDISIG::MIDISIG_END:
        ASSERT(byte_len == 0);
        is_midisegment_end = true;
        break;
      default:
        // do nothing
        stream.GetChar(buf_midi, byte_len);
      }
    } while (!is_midisegment_end);
    trackidx ++;
  } while (!stream.IsEnd());

  free(buf_midi);
  return true;
}

} /* end rparser */