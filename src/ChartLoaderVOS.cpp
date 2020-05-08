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
#include "common.h"

#define MAX_READ_SIZE 512000
//constexpr double kVOSTimeConstant = 0.6;
constexpr double kVOSTimeConstant = 0.156;

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
  uint8_t dummy;        // always zero
  uint32_t time;
  uint16_t pitch;
  uint8_t volume;
  uint8_t istappable;   // 0 or 1
  uint8_t issoundable;  // soundable; always 1?
  uint8_t islongnote;
  uint32_t duration;
  uint8_t dummy2;

  // -- additional information --
  uint8_t lane;
  uint8_t inst;
};

struct VOSNoteDataV3 {
  uint32_t time;
  uint32_t duration;
  uint8_t midicmd;
  uint8_t midikey;
  uint8_t vol;
  uint16_t type;      /* (Bit) 8 : tapnote, 9 : longnote, 15 : longnote */
};

#define MIDISIG_TYPES 8
#define MIDISIG_LENGTH 2
#define MIDISIG_NULLCMPR 0xFF /* this byte wont be compared */

// https://www.csie.ntu.edu.tw/~r92092/ref/midi/#sysex_event
enum class MIDISIG: int {
  MIDISIG_DUMMY=0, // output port
  MIDISIG_TRACKNAME,
  MIDISIG_END,
  MIDISIG_TEMPO,
  MIDISIG_MEASURE,
  MIDISIG_META_OTHERS,
  MIDISIG_SYSEX,
  // -- below is for program --
  MIDISIG_PROGRAM,
  MIDISIG_PROGRAM_RUNNING,
  MIDISIG_OTHERS
};

struct MIDIProgramInfo {
  uint8_t lastcmd, cmd, a, b;
  int len;
  std::string text;

  MIDIProgramInfo()
  {
    lastcmd = cmd = a = b = 0;
    len = 0;
  }
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
  ASSERT(offset_ <= len_);
}

void ChartLoaderVOS::BinaryStream::SeekCur(size_t cnt)
{
  offset_ += cnt;
  ASSERT(offset_ <= len_);
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
  ASSERT(offset_ <= len_);
  return r;
}

uint32_t ChartLoaderVOS::BinaryStream::GetUInt32()
{
  uint32_t r = *reinterpret_cast<const uint32_t*>((uint8_t*)p_ + offset_);
  offset_ += sizeof(uint32_t);
  ASSERT(offset_ <= len_);
  return r;
}

uint16_t ChartLoaderVOS::BinaryStream::GetUInt16()
{
  uint16_t r = *reinterpret_cast<const uint16_t*>((uint8_t*)p_ + offset_);
  offset_ += sizeof(uint16_t);
  ASSERT(offset_ <= len_);
  return r;
}

uint8_t ChartLoaderVOS::BinaryStream::GetUInt8()
{
  uint8_t r = *reinterpret_cast<const uint8_t*>((uint8_t*)p_ + offset_);
  offset_ += sizeof(uint8_t);
  ASSERT(offset_ <= len_);
  return r;
}

void ChartLoaderVOS::BinaryStream::GetChar(char *out, size_t cnt)
{
  memcpy(out, (uint8_t*)p_ + offset_, cnt);
  offset_ += cnt;
  ASSERT(offset_ <= len_);
}

size_t ChartLoaderVOS::BinaryStream::GetOffset()
{
  return offset_;
}

inline int GetMSFixedInt_internal(const uint8_t *p, uint8_t len)
{
  int r = 0;
  for (int i = 0; i < len; i++)
  {
    r = (r << 8) + *p;
    p++;
  }
  return r;
}

int ChartLoaderVOS::BinaryStream::GetMSFixedInt(uint8_t bytesize)
{
  int r = GetMSFixedInt_internal((uint8_t*)p_ + offset_, bytesize);
  offset_ += bytesize;
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
  MIDISIG r = MIDISIG::MIDISIG_OTHERS;
  mprog.cmd = GetUInt8();
  mprog.a = mprog.b = 0;
  mprog.len = 0;
  mprog.text.clear();
  char buf[256];

  // check SYSEX event first
  if (mprog.cmd == 0xF0 || mprog.cmd == 0xF7)
  {
    // XXX: informing necessary?
    std::cerr << "MIDI Sysex event found, going to be ignored." << std::endl;
    mprog.len = GetMSInt();
    GetChar(buf, mprog.len);
    mprog.text = std::string(buf, mprog.len);
    return MIDISIG::MIDISIG_SYSEX;
  }

  if (mprog.cmd < 0x80)
  {
    // running status. use previous command
    ASSERT(mprog.lastcmd > 0);
    mprog.a = mprog.cmd;
    mprog.cmd = mprog.lastcmd;
  }
  else
  {
    mprog.lastcmd = mprog.cmd;
    mprog.a = GetUInt8();
  }

  // check for meta event
  if (mprog.cmd == 0xFF)
  {
    // basic form of meta event
    mprog.len = GetMSInt();
    GetChar(buf, mprog.len);
    mprog.text = std::string(buf, mprog.len);

    // check for some special commands (TEMPO, EOT)
    if (mprog.a == 0x2F) return MIDISIG::MIDISIG_END;
    else if (mprog.a == 0x51) return MIDISIG::MIDISIG_TEMPO;
    else if (mprog.a == 0x58) return MIDISIG::MIDISIG_MEASURE;
    else return MIDISIG::MIDISIG_META_OTHERS;
  }

  // check voice message: if it is necessary to parse second parameter
  switch ((mprog.cmd >> 4) & 0x07)
  {
  case 0: /* Note off */
  case 1: /* Note on */
  case 2: /* Key pressure */
  case 3: /* Control change */
  case 6: /* Pitch wheel */
    mprog.b = GetUInt8();
    break;
  }

#if 0
  if (mprog.cmd < 0xF0)
    r = MIDISIG::MIDISIG_PROGRAM;
#endif

  return r;
}

bool ChartLoaderVOS::BinaryStream::IsEnd()
{
  return offset_ >= len_;
}

ChartLoaderVOS::ChartLoaderVOS(Song *song)
  : ChartLoader(song), vos_version_(VOS_UNKNOWN) {};

bool ChartLoaderVOS::LoadFromDirectory()
{
  return false;
}

bool ChartLoaderVOS::Load( Chart &c, const void* p, unsigned len ) {
  Preload(c, p, len);
  this->chart_ = &c;
  stream.SetSource(p, len);
  timedivision_ = 120;

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

  // common part: 16 channels for midi
  for (size_t i = 0; i < 16; ++i)
    md.GetSoundChannel()->fn[i] = "midi";

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

  // common part: 16 channels for midi
  for (size_t i = 0; i < 16; ++i)
    md.GetSoundChannel()->fn[i] = "midi";

  int headersize = stream.GetInt32();
  stream.SeekCur(4);  // inf; static flag
  stream.SeekCur(12); // inf offset (not exact)
  vos_v3_midi_offset_ = stream.GetInt32(); // end of inf file pos (start of MID file)
  stream.SeekCur(4);  // mid; static flag
  stream.SeekCur(12); // midi offset (not exact)
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
    static const char *metadatas1[] = { "ARTIST", "TITLE", "SUBARTIST", "GENRE", "CHARTMAKER" };
    metadatas = metadatas1;
    metadata_cnt = 5;
  }
  else
  {
    static const char *metadatas1[] = { "ARTIST", "TITLE", "SUBARTIST", "CHARTMAKER" };
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
  std::vector<VOSNoteDataV2*> vnotes;
  VOSNoteDataV2* vnote_tappable[65536];
  VOSNoteDataV2* vnote;
  size_t _tappable_cnt = 0;
  auto &nd = chart_->GetNoteData();
  auto &bgm = chart_->GetBgmData();
  for (int i=0; i<cnt_inst; i++) {
    int channel = i;
    int notecnt = stream.GetInt32();
    for (int j=0; j<notecnt; j++) {
      vnote = new VOSNoteDataV2();
      /** cannot do memcpy due to struct padding. */
      vnote->dummy = stream.GetUInt8();       // always zero
      vnote->time = static_cast<uint32_t>(stream.GetUInt32() * kVOSTimeConstant);
      vnote->pitch = stream.GetUInt16();
      vnote->volume = stream.GetUInt8();
      vnote->istappable = stream.GetUInt8();
      vnote->issoundable = stream.GetUInt8();
      vnote->islongnote = stream.GetUInt8();
      vnote->duration = static_cast<uint32_t>(stream.GetUInt32() * kVOSTimeConstant);
      vnote->dummy2 = stream.GetUInt8();      // 00 ~ 04? unknown
      vnote->lane = 0;
      vnote->inst = channel;
      vnotes.push_back(vnote);
      if (vnote->istappable)
        vnote_tappable[_tappable_cnt++] = vnote;
    }
  }

  /** Actual playable note segment */
  ASSERT(stream.ReadInt32() == 0);
  stream.SeekCur(4);                      // empty int32
  uint32_t seg_cnt = stream.GetUInt32();  // note segment count
  ASSERT(seg_cnt == _tappable_cnt);

  for (size_t i = 0; i < seg_cnt; i++)
  {
    uint8_t inst_idx = stream.GetUInt8();   // seems like instrument channel?
    uint32_t idx = stream.GetUInt32();      // idx for instrument channel
    uint8_t lane = stream.GetUInt8();
    //std::cout << (int)lane << "," << (int)ln_size << ",idx:" << (int)idx << std::endl;
    vnote_tappable[i]->lane = lane;
  }
  stream.SeekCur(4);                      // some unknown bytes

  uint32_t lyrics_cnt = stream.GetUInt32();
  for (size_t i = 0; i < lyrics_cnt; i++)
  {
    uint32_t time = stream.GetUInt32();
    uint8_t lyric_len = stream.GetUInt8();
    stream.GetChar(buf, lyric_len);
  }
  
  /** Append data to NoteData and cleanup */
  NoteElement ne;
  for (auto *p : vnotes)
  {
    unsigned track = p->lane;
    if (p->istappable)
    {
      // midi time --> beat position
      ne.set_measure(p->time / (double)timedivision_ / 4.0);
      auto &sprop = ne.get_property_sound();
      sprop.volume = p->volume / 127.0f;
      sprop.key = p->pitch;
      sprop.length = p->duration;
      ne.set_value(p->inst);
      if (p->islongnote)
      {
        ne.set_chain_status(NoteChainStatus::Start);
        nd[track].AddNoteElement(ne);
        // XXX: length is not correct ...
        ne.set_measure(ne.measure() + p->duration / (double)timedivision_ / 4.0);
        ne.set_chain_status(NoteChainStatus::End);
        nd[track].AddNoteElement(ne);
      }
      else
      {
        ne.set_chain_status(NoteChainStatus::Tap);
        nd[track].AddNoteElement(ne);
      }
    }
    else
    {
      ne.set_measure(p->time / (double)timedivision_ / 4.0);
      auto &sprop = ne.get_property_sound();
      sprop.volume = p->volume / 127.0f;
      sprop.key = p->pitch;
      sprop.length = p->duration;
      ne.set_value(p->inst);
      bgm[0].AddNoteElement(ne);
    }
  }
  for (auto* p : vnotes)
    delete p;

  return true;
}

bool ChartLoaderVOS::ParseNoteDataV3()
{
  // start to read note data
  VOSNoteDataV3 note;
  NoteElement ne;
  uint32_t cnt;
  auto &nd = chart_->GetNoteData();
  auto &bgm = chart_->GetBgmData();
  size_t segment_idx = 0;   // maybe midi channel no?

  while (stream.GetOffset() < vos_v3_midi_offset_) {
    int midiinstrument = stream.GetInt32();
    cnt = stream.GetUInt32();
    stream.SeekCur(14);

    for (unsigned i=0; i<cnt; i++) {
      note.time = static_cast<uint32_t>(stream.GetUInt32() * kVOSTimeConstant);
      note.duration = static_cast<uint32_t>(stream.GetUInt32() * kVOSTimeConstant);
      note.midicmd = stream.GetUInt8();
      note.midikey = stream.GetUInt8();
      note.vol = stream.GetUInt8();
      note.type = stream.GetUInt16();

      uint8_t istappable = (note.type & 0b010000000) > 0;
      uint8_t islongnote = (note.type & 0b01000000000000000) > 0;
      uint8_t keybits = (note.type >> 4) & 0b0111;
      bool is_tap = (segment_idx == 16 && istappable);
      unsigned channel = (note.midicmd & 0x0F);

      ne.set_measure(note.time / (double)timedivision_ / 4.0);
      auto &sprop = ne.get_property_sound();
      sprop.volume = note.vol / 127.0f;
      sprop.key = note.midikey;
      sprop.length = note.duration;
      ne.set_value(channel);
      //sprop.channel = channel;

      if (is_tap)
      {
        if (islongnote)
        {
          ne.set_chain_status(NoteChainStatus::Start);
          nd[keybits].AddNoteElement(ne);
          // XXX: need to be fixed
          ne.set_chain_status(NoteChainStatus::End);
          ne.set_measure(ne.measure() + note.duration / (double)timedivision_ / 4.0);
          nd[keybits].AddNoteElement(ne);
        }
        else
        {
          ne.set_chain_status(NoteChainStatus::Tap);
          nd[keybits].AddNoteElement(ne);
        }
      }
      else
      {
        bgm[0].AddNoteElement(ne);
      }
    }
    segment_idx++;
  }
  ASSERT(segment_idx == 17);

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
  uint32_t timedivision = stream.GetUInt8();  // tick size
  if (timedivision_ != timedivision)
  {
    double ratio = (double)timedivision_ / timedivision;
    auto alliter = chart_->GetNoteData().GetRowIterator();
    while (!alliter.is_end())
    {
      for (size_t i = 0; i < chart_->GetNoteData().get_track_count(); ++i)
        if (alliter.get(i)) alliter[i].set_measure(alliter[i].measure() * ratio);
      ++alliter;
    }
    timedivision_ = timedivision;
  }
  double cur_measure = 0;

  auto &td = chart_->GetTimingData();
  auto &nd = chart_->GetNoteData();
  auto &cd = chart_->GetCommandData();
  uint32_t val, val2;
  NoteElement ne;

  int trackidx = 0;
  do {  /* loop for each MIDI file */
    stream.GetChar(buf, 4);
    if (memcmp(buf, "MTrk", 4) != 0)
      break;

    uint32_t tracksize = stream.GetMSFixedInt();
    uint32_t offset_cur = stream.GetOffset();

    MIDISIG midisig = MIDISIG::MIDISIG_DUMMY;
    MIDIProgramInfo mprog;
    int delta = 0;  // delta tick(position) from previous event.
    int ticks = 0;  // current midi event position (tick). 480 tick: 1 measure (4 beat), mostly.
    bool is_midisegment_end = false;

    do { /* loop for each MIDI command in MIDI file */
      delta = stream.GetMSInt();
      midisig = stream.GetMidiSignature(mprog);
      ticks += delta;
      cur_measure = (double)ticks / timedivision / 4.0;
      ne.set_measure(cur_measure);
#if 0
      std::cout << (int)ticks << "/" << (int)mprog.cmd << " "
        << (int)mprog.a << " "
        << (int)mprog.b << std::endl;
#endif
      ASSERT(stream.GetOffset() <= offset_cur + tracksize);

      switch (midisig)
      {
      case MIDISIG::MIDISIG_TEMPO:
        val = GetMSFixedInt_internal((uint8_t*)mprog.text.c_str(), 3);
        ne.set_value(60'000'000 / timedivision * 120 / (double)val);
        td[TimingTrackTypes::kBpm].AddNoteElement(ne);
        break;
      case MIDISIG::MIDISIG_MEASURE:
        val = mprog.text.c_str()[0];
        val2 = mprog.text.c_str()[1];
        ne.set_value(val / val2);
        td[TimingTrackTypes::kMeasure].AddNoteElement(ne);
        break;
      case MIDISIG::MIDISIG_END:
        is_midisegment_end = true;
        break;
      case MIDISIG::MIDISIG_SYSEX:
        // ignore
        break;
      case MIDISIG::MIDISIG_META_OTHERS:
        // ignore other metas
        break;
      case MIDISIG::MIDISIG_PROGRAM:
      case MIDISIG::MIDISIG_OTHERS:
        // all of other metas & program info
        ne.set_value(trackidx);
        ne.set_point(mprog.cmd, mprog.a, mprog.b);
        cd[CommandTrackTypes::kMidi].AddNoteElement(ne);
        break;
      default:
        ASSERT(0);
      }
    } while (!is_midisegment_end);
    trackidx ++;
  } while (!stream.IsEnd());

  free(buf_midi);
  return true;
}

} /* end rparser */
