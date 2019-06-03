/* supports bms, bme, bml formats. */

#include "ChartLoader.h"
#include "Chart.h"
#include "rutil.h"

using namespace rutil;

namespace rparser {

ChartLoaderBMS::LineContext::LineContext() { clear(); }
void ChartLoaderBMS::LineContext::clear()
{
  stmt = 0;
  stmt_len = 0;
  *command = 0;
  value = 0;
  value_len = 0;
  measure = 0;
  bms_channel = 0;
}

ChartLoaderBMS::ChartLoaderBMS()
{

}

bool TestName( const char *fn )
{
  std::string fn_lower = fn;  lower(fn_lower);
  return endsWith(fn_lower, ".bms") ||
         endsWith(fn_lower, ".bme") ||
         endsWith(fn_lower, ".bml") ||
         endsWith(fn_lower, ".pms");
}

bool ChartLoaderBMS::LoadFromDirectory(ChartListBase& chartlist, Directory& dir)
{
  if (dir.count() <= 0)
    return false;

  for (auto &f : dir)
  {
    if (!dir.Read(f.d)) continue;
    const std::string filename = f.d.GetFilename();
    if (!TestName(filename.c_str())) continue;

    Chart *c = chartlist.GetChartData(chartlist.AddNewChart());
    if (!c) return false;

    c->SetFilename(filename);
    c->SetHash(rutil::md5_str(f.d.GetPtr(), f.d.GetFileSize()));
    c->GetTempoData().SetMeasureLengthRecover(true);  // enable by default
    bool r = Load(*c, f.d.p, f.d.len);

    chartlist.CloseChartData();

    if (!r)
    {
      std::cerr << "Failed to read chart file (may be invalid) : " << filename << std::endl;
      chartlist.DeleteChart(chartlist.size() - 1);
    }
  }

  return true;
}

inline bool IsCharacterTrimmable(char c)
{
  return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

bool ChartLoaderBMS::Load(Chart &c, const void* p, int iLen)
{
  /** Initialize global context */
  const char* const chr = static_cast<const char*>(p);
  chart_context_ = &c;
  memset(longnote_idx_per_lane, 0xffffffff, sizeof(longnote_idx_per_lane));
  bga_column_idx_per_measure_.clear();

  size_t pos = 0;
  size_t stmtlen = 0;
  size_t nextpos = 0;
  while (pos < iLen)
  {
    if (!IsCharacterTrimmable(chr[pos]))
    {
      stmtlen = 0;
      while (pos + stmtlen < iLen && chr[pos + stmtlen] != '\n')
        stmtlen++;
      nextpos = stmtlen + pos + 1;
      while (stmtlen > 0 && IsCharacterTrimmable(chr[pos + stmtlen - 1]))
        stmtlen--;

      current_line_.clear();
      current_line_.stmt = chr + pos;
      current_line_.stmt_len = stmtlen;

      if (!ParseCurrentLine())
      {
        // XXX: for debug
        std::string de(current_line_.stmt, current_line_.stmt_len);
        std::cerr << "parse failed: " << de.c_str() << std::endl;
      }

      pos = nextpos;
    } else pos++;
  }

  return true;
}

// main file
bool ChartLoaderBMS::Test(const void* p, int iLen)
{
  // As bms file has no file signature,
  // just search for "*--" string for first 100 string.
  // TODO: is there any exception?
  for (int i = 0; i<iLen - 4 && i<100; i++) {
    if (strncmp((const char*)p, "*-----", 6) == 0)
      return true;
  }
  return false;
}

inline char upperchr(char c)
{
  return (c > 96 && c < 123) ? (c ^ 0x20) : c;
}

unsigned int atoi_bms_measure(const char* p, unsigned int length = 3)
{
  unsigned int r = 0;
  while (p && length)
  {
    r *= 10;
    if (*p >= '0' && *p <= '9')
      r += (*p-'0');
    else break;
    ++p; --length;
  }
  return r;
}

unsigned int atoi_bms_channel(const char* p, unsigned int length = 2)
{
  unsigned int r = 0;
  while (*p && length)
  {
    r *= 26+10;
    if (*p >= 'a' && *p <= 'z')
      r += 10+(*p-'a');
    else if (*p >= 'A' && *p <= 'Z')
      r += 10+(*p-'A');
    else if (*p >= '0' && *p <= '9')
      r += (*p-'0');
    else break;
    ++p; --length;
  }
  return r;
}

#if 0
int atoi_bms16(const char* p, int length)
{
  // length up-to-2
  int r = 0;
  while (p && length)
  {
    r *= 16;
    if (*p >= 'a' && *p <= 'f')
      r += 10+(*p-'a');
    else if (*p >= 'A' && *p <= 'F')
      r += 10+(*p-'A');
    else if (*p >= '0' && *p <= '9')
      r += (*p-'0');
    else break;
    ++p; --length;
  }
  return r;
}
#endif

bool ChartLoaderBMS::ParseCurrentLine()
{
  const char *c, *p;
  size_t len = current_line_.stmt_len;
  char *cw = current_line_.command;
  char terminator_type;
  c = p = current_line_.stmt;

  // check field separation message
  if (current_line_.stmt_len > 23 &&
      memcmp(current_line_.stmt, "*----------------------", 23) == 0)
    return true;

  // if not header field
  if (*c != '#') return false;
  c++;
  while (*c != ' ' && *c != ':' && c-p < len)
  {
    *cw = upperchr(*c);
    c++; cw++;
  }
  *cw = 0;
  // check an attribute has no value
  if (c - p == len)
  {
    current_line_.value_len = 0;
    terminator_type = ' ';
  }
  else
  {
    current_line_.value_len = len-(c-p)-1;
    terminator_type = *c;
  }
  current_line_.value = c + 1;

  if (terminator_type == ':')
  {
    current_line_.measure = atoi_bms_measure(current_line_.command, 3);
    current_line_.bms_channel = atoi_bms_channel(current_line_.command + 3, 2);
    if (!ParseNote())
      return false;
  }
  else if (terminator_type == ' ')
  {
    if (!ParseControlFlow() && !ParseMetaData())
      return false;
  }
  else return false;

  return true;
}

#define CMDCMP(x) (strcmp(current_line_.command, (x)) == 0)
bool ChartLoaderBMS::ParseControlFlow()
{
  unsigned int cond = atoi_bms_measure(current_line_.value, current_line_.value_len);

  if (CMDCMP("SWITCH"))
  {
    // check start of the stmt
    chart_context_stack_.push_back(chart_context_);
    condstmt_ = chart_context_->CreateStmt(cond, true, false);
    chart_context_ = 0;
  }
  else if (CMDCMP("RANDOM") || CMDCMP("RONDOM") || CMDCMP("SETRANDOM"))
  {
    // check start of the stmt
    chart_context_stack_.push_back(chart_context_);
    condstmt_ = chart_context_->CreateStmt(cond, false, true);
    chart_context_ = 0;
  }
  else if (CMDCMP("ENDRANDOM") || CMDCMP("ENDRONDOM"))
  {
    // restore chart context.
    chart_context_ = chart_context_stack_.back();
    condstmt_ = chart_context_->GetLastStmt();
    chart_context_stack_.pop_back();
  }
  else if (CMDCMP("IF"))
  {
    /**
     * COMMENT: IF statement can exist multiple times in same RANDOM block
     * So comment out assert below:
     * ASSERT(condstmt_->GetSentenceCount() == 0);
     */
    // TODO: no ELSE statement should exist
    chart_context_ = condstmt_->CreateSentence(cond);
  }
  else if (CMDCMP("ELSE"))
  {
    ASSERT(condstmt_->GetSentenceCount() > 0);
    chart_context_ = condstmt_->CreateSentence(-1);
  }
  else if (CMDCMP("ELSEIF"))
  {
    // TODO: no ELSE statement should exist
    ASSERT(condstmt_->GetSentenceCount() > 0);
    chart_context_ = condstmt_->CreateSentence(cond);
  }
  else if (CMDCMP("ENDIF") || CMDCMP("END"))
  {
    // just clear chart context (unparse-able state)
    ASSERT(condstmt_->GetSentenceCount() > 0);
    chart_context_ = 0;
  }
  else return false;
  return true;
}
#undef CMDCMP

bool ChartLoaderBMS::ParseMetaData()
{
  // check contains value
  if (current_line_.value_len == 0)
  {
    std::cerr << "Warning: Command #" << current_line_.command << " has no value, ignored." << std::endl;
    return true;
  }

  // cannot parse between control flow stmt
  if (!chart_context_) return false;
  MetaData& md = chart_context_->GetMetaData();
  std::string cmd(current_line_.command);
  std::string value(current_line_.value, current_line_.value_len);

  /**
   * @warn
   * LNTYPE metadata effects directly in syntax progress!
   */
  if (cmd == "LNTYPE")
    md.bms_longnote_type = atoi(value.c_str());
  else if (cmd == "LNOBJ")
    md.bms_longnote_object = atoi(value.c_str());

# define CHKCMD(cmd_const, len) (strncmp(current_line_.command, cmd_const, len) == 0)
  if (CHKCMD("BMP",3)) {
    unsigned int key = atoi_bms_channel(current_line_.value + 3);
    md.GetBGAChannel()->bga[key] = { value, 0,0,0,0, 0,0,0,0 };
  }
  else if (CHKCMD("WAV",3)) {
    unsigned int key = atoi_bms_channel(current_line_.value + 3);
    md.GetSoundChannel()->fn[key] = value;
  }
  else if (cmd.size() > 3 && (CHKCMD("EXBPM",5) || CHKCMD("BPM",3))) {
    unsigned int key;
    if (cmd[0] == 'B')
        key = atoi_bms_channel(cmd.c_str() + 3);
    else
        key = atoi_bms_channel(cmd.c_str() + 5);
    md.GetBPMChannel()->bpm[key] = static_cast<float>(atof( value.c_str() ));
  }
  else if (CHKCMD("STOP",4)) {
    unsigned int key = atoi_bms_channel(current_line_.value  + 4);
    md.GetSTOPChannel()->stop[key] = static_cast<float>(atoi(value.c_str()));  // 1/192nd
  }
  // TODO: WAVCMD, EXWAV
  // TODO: MIDIFILE
# undef CHKCMD
  else if (cmd == "STP") {
    std::string sMeasure, sTime;
    if (!split(value, ' ', sMeasure, sTime))
    {
      printf("invalid #STP found, ignore. (%s)\n", value.c_str());
      return false;
    }
    float fMeasure = static_cast<float>(atof(sMeasure.c_str()));
    md.GetSTOPChannel()->STP[fMeasure] = static_cast<float>(atoi(sTime.c_str()));
  }
  else {
    md.SetAttribute(cmd, value);
  }

  return true;
}

constexpr unsigned int radix_16_2_36(unsigned int radix_16)
{
  return (radix_16 / 16) * 36 + (radix_16 % 16);
}

int GetNoteTypeFromBmsChannel(unsigned int bms_channel)
{
  switch (bms_channel)
  {
  case 1:   // BGM
    return NoteTypes::kBGM;
  case 3:   // BPM change
  case 8:   // BPM (exbpm)
  case 9:   // STOP
    return NoteTypes::kTempo;
  case 4:   // BGA
  case 6:   // BGA poor
  case 7:   // BGA layered
  case 10:  // BGA layered 2
  case 11:  // BGA opacity
  case 12:  // BGA opacity layer
  case 13:  // BGA opacity layer 2
  case 14:  // BGA opacity poor
    return NoteTypes::kEvent;
  default:
    // 1P/2P visible note
    if (bms_channel >= radix_16_2_36(0x11) && bms_channel <= radix_16_2_36(0x19) ||
        bms_channel >= radix_16_2_36(0x21) && bms_channel <= radix_16_2_36(0x29))
      return NoteTypes::kTap;
    // 1P/2P invisible note
    else if (bms_channel >= radix_16_2_36(0x31) && bms_channel <= radix_16_2_36(0x39) ||
             bms_channel >= radix_16_2_36(0x41) && bms_channel <= radix_16_2_36(0x49))
      return NoteTypes::kTap;
    // Longnote
    else if (bms_channel >= radix_16_2_36(0x51) && bms_channel <= radix_16_2_36(0x59) ||
             bms_channel >= radix_16_2_36(0x61) && bms_channel <= radix_16_2_36(0x69))
      return NoteTypes::kTap;
    // Mine
    else if (bms_channel >= radix_16_2_36(0xD1) && bms_channel <= radix_16_2_36(0xD9) ||
             bms_channel >= radix_16_2_36(0xE1) && bms_channel <= radix_16_2_36(0xE9))
      return NoteTypes::kTap;
  }
  return NoteTypes::kNone;
}

int GetNoteSubTypeFromBmsChannel(unsigned int bms_channel)
{
  switch (bms_channel)
  {
  case 1:   // BGM (no subtype)
    return 0;
  case 3:   // BPM change
    return NoteTempoTypes::kBpm;
  case 8:   // BPM (exbpm)
    return NoteTempoTypes::kBmsBpm;
  case 9:   // STOP
    return NoteTempoTypes::kBmsStop;
  case 4:   // BGA
  case 6:   // BGA poor
  case 7:   // BGA layered
  case 10:  // BGA layered 2
    return NoteEventTypes::kBGA;
  case 11:  // BGA opacity
  case 12:  // BGA opacity layer
  case 13:  // BGA opacity layer 2
  case 14:  // BGA opacity poor
    return NoteEventTypes::kBmsARGBLAYER;
  default:
    // 1P/2P visible note
    if (bms_channel >= radix_16_2_36(0x11) && bms_channel <= radix_16_2_36(0x19) ||
        bms_channel >= radix_16_2_36(0x21) && bms_channel <= radix_16_2_36(0x29))
      return NoteSubTypes::kNormalNote;
    // 1P/2P invisible note
    else if (bms_channel >= radix_16_2_36(0x31) && bms_channel <= radix_16_2_36(0x39) ||
             bms_channel >= radix_16_2_36(0x41) && bms_channel <= radix_16_2_36(0x49))
      return NoteSubTypes::kInvisibleNote;
    // Longnote
    else if (bms_channel >= radix_16_2_36(0x51) && bms_channel <= radix_16_2_36(0x59) ||
             bms_channel >= radix_16_2_36(0x61) && bms_channel <= radix_16_2_36(0x69))
      return NoteSubTypes::kLongNote;
    // Mine
    else if (bms_channel >= radix_16_2_36(0xD1) && bms_channel <= radix_16_2_36(0xD9) ||
             bms_channel >= radix_16_2_36(0xE1) && bms_channel <= radix_16_2_36(0xE9))
      return NoteSubTypes::kMineNote;
  }
  return 0;
}

uint8_t GetNotePlayerFromBmsChannel(unsigned int bms_channel)
{
  if (bms_channel >= radix_16_2_36(0x21) && bms_channel <= radix_16_2_36(0x29) ||
      bms_channel >= radix_16_2_36(0x41) && bms_channel <= radix_16_2_36(0x49) ||
      bms_channel >= radix_16_2_36(0x61) && bms_channel <= radix_16_2_36(0x69) ||
      bms_channel >= radix_16_2_36(0xE1) && bms_channel <= radix_16_2_36(0xE9))
    return 1;   // 2P
  return 0;     // 1P
}

/* SC is 8th ch */
/* in DP: SC is 15, 16th ch */
/* XXX: pedal is 9th ch, but when it would be in progress? */
uint8_t GetNoteColFromBmsChannel(unsigned int bms_channel)
{
  if (bms_channel < radix_16_2_36(0x20)) return 0;  // special channel
  unsigned int channel_mod_16 = bms_channel % 36;
  if (channel_mod_16 >= 16) return 0;
  unsigned int channel_remap[16] = 
  {0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 0, 0, 0, 0, 0, 0};
  return channel_remap[channel_mod_16];
}

bool ChartLoaderBMS::ParseMeasureLength()
{
  std::string v(current_line_.value, current_line_.value_len);
  TempoNote n;
  n.SetRowPos(current_line_.measure, 0, 0);
  n.SetMeasure(atof(v.c_str()) * kDefaultMeasureLength);
  chart_context_->GetTempoData().GetTempoNoteData().AddNote(n);
  return true;
}

bool ChartLoaderBMS::ParseNote()
{
  bool r = false;
  const unsigned int channel = current_line_.bms_channel;
  unsigned int len = current_line_.value_len;

  // cannot parse between control flow stmt
  if (!chart_context_) return false;

  // check for measure length
  // - if measure changed, 
  if (channel == 2)
    return ParseMeasureLength();

  // warn for incorrect length
  if (len % 2 == 1)
  {
    std::cerr << "Warning: incorrect measure length detected. (" << current_line_.command << ")" << std::endl;
    len--;
  }
  if (len == 0) return false;

  // parse various types of note
  switch (GetNoteTypeFromBmsChannel(current_line_.bms_channel))
  {
  case NoteTypes::kBGM:
  case NoteTypes::kTap:
  case NoteTypes::kTouch:
    r = ParseSoundNote();
    break;
  case NoteTypes::kTempo:
    r = ParseTempoNote();
    break;
  case NoteTypes::kEvent:
    r = ParseCommandNote();
    break;
  }

  return r;
}

bool ChartLoaderBMS::ParseCommandNote()
{
  const unsigned int measure = current_line_.measure;
  const unsigned int channel = current_line_.bms_channel;
  const char* value = current_line_.value;
  unsigned int len = current_line_.value_len;
  unsigned int value_u;
  static uint8_t bga_per_bms_channel[] = { 4, 6, 7, 10 };
  static uint8_t bmsargb_per_bms_channel[] = { 11, 12, 13, 14 };
  EventNote n;

  n.SetDenominator(len);
  n.set_type(GetNoteTypeFromBmsChannel(channel));
  n.set_subtype(GetNoteSubTypeFromBmsChannel(channel));
  for (unsigned int i = 0; i < len; i += 2)
  {
    n.SetRowPos(measure, len, i);
    value_u = atoi_bms_channel(value + i);
    if (value_u == 0) continue;
    int bgatypes;

    switch (n.subtype())
    {
    case NoteEventTypes::kBGA:
      for (bgatypes = 0; bgatypes < 4; bgatypes++)
        if (bga_per_bms_channel[bgatypes] == channel) break;
      ASSERT(bgatypes < 4);
      n.SetBga((BgaTypes)bgatypes, value_u, bga_column_idx_per_measure_[measure]++);
      break;
    case NoteEventTypes::kBmsARGBLAYER:
      for (bgatypes = 0; bgatypes < 4; i++)
        if (bmsargb_per_bms_channel[bgatypes] == channel) break;
      ASSERT(bgatypes < 4);
      n.SetBmsARGBCommand((BgaTypes)bgatypes, value_u);
      break;
    default:
      ASSERT(0);
    }

    chart_context_->GetEventNoteData().AddNote(n);
  }

  return true;
}

bool ChartLoaderBMS::ParseSoundNote()
{
  unsigned int value_u;
  SoundNote n;
  const unsigned int measure = current_line_.measure;
  const unsigned int channel = current_line_.bms_channel;
  const char* value = current_line_.value;
  unsigned int len = current_line_.value_len;
  bool register_longnote;
  uint32_t curlane;

  memset(&n, 0, sizeof(SoundNote)); // XXX: is this okay?
  NotePos& npos = n.pos();
  n.SetDenominator(len);
  n.set_type(GetNoteTypeFromBmsChannel(channel));
  n.set_subtype(GetNoteSubTypeFromBmsChannel(channel));
  n.track.lane.note.player = GetNotePlayerFromBmsChannel(channel);
  n.track.lane.note.lane = curlane = GetNoteColFromBmsChannel(channel);
  for (unsigned int i = 0; i < len; i += 2)
  {
    value_u = atoi_bms_channel(value+i);
    if (value_u == 0)
    {
      /** Close longnote in case of LNTYPE 2 (obsolete) */
      if (chart_context_->GetMetaData().bms_longnote_type == 2)
        longnote_idx_per_lane[channel] = UINT32_MAX;
      /** Don't do anything in 00 note */
      continue;
    }

    npos.SetRowPos(measure, len, i);
    n.value = static_cast<Channel>(value_u);

    /** Longnote check */
    register_longnote = false;
    if ((n.type() == NoteTypes::kTap && n.subtype() == NoteSubTypes::kLongNote) /* General LN channel */ ||
        (value_u == chart_context_->GetMetaData().bms_longnote_object) /* LNOBJ check */)
    {
      /** LNTYPE 2 (obsolete) */
      if (chart_context_->GetMetaData().bms_longnote_type == 2)
      {
        if (longnote_idx_per_lane[curlane] == UINT32_MAX)
          longnote_idx_per_lane[curlane] = chart_context_->GetNoteData().size();
        else
        {
          SoundNote& ln = chart_context_->GetNoteData().get(longnote_idx_per_lane[curlane]);
          ln.SetLongnoteEndValue(value_u);
          ln.SetLongnoteEndPos(npos);
          continue; /* Don't add note */
        }
      }
      /** LNTYPE 1 (default) or LNOBJ */
      else
      {
        if (longnote_idx_per_lane[curlane] == UINT32_MAX)
          longnote_idx_per_lane[curlane] = chart_context_->GetNoteData().size();
        else
        {
          SoundNote& ln = chart_context_->GetNoteData().get(longnote_idx_per_lane[curlane]);
          ln.SetLongnoteEndValue(value_u);
          ln.SetLongnoteEndPos(npos);
          longnote_idx_per_lane[curlane] = UINT32_MAX;
          continue; /* Don't add note */
        }
      }
    }

    chart_context_->GetNoteData().AddNote(n);
  }

  return true;
}

bool ChartLoaderBMS::ParseTempoNote()
{
  unsigned int value_u;
  const unsigned int measure = current_line_.measure;
  const unsigned int channel = current_line_.bms_channel;
  const char* value = current_line_.value;
  unsigned int len = current_line_.value_len;
  TempoNote n;

  n.SetDenominator(len);
  n.set_type(GetNoteTypeFromBmsChannel(channel));
  n.set_subtype(GetNoteSubTypeFromBmsChannel(channel));
  for (unsigned int i = 0; i < len; i += 2)
  {
    value_u = atoi_bms_channel(value + i);
    if (value_u == 0) continue;

    n.SetRowPos(measure, len, i);
    switch (n.subtype())
    {
    case NoteTempoTypes::kBpm:
      /** 16 radix here */
      n.SetBpm(atoi_16(value + i, 2));
      break;
    case NoteTempoTypes::kBmsBpm:
      n.SetBmsBpm(value_u);
      break;
    case NoteTempoTypes::kBmsStop:
      n.SetBmsStop(value_u);
      break;
    default:
      ASSERT(0);
    }

    chart_context_->GetTempoData().GetTempoNoteData().AddNote(n);
  }

  return true;
}

} /* rparser */
