/* supports bms, bme, bml formats. */

#include "ChartLoader.h"
#include "Chart.h"
#include "rutil.h"
#include "common.h"

using namespace rutil;

namespace rparser {

enum class BmsChannelTypes
{
  kNone,
  kBgm,
  kBga,
  kNote,
  kTiming,
  kEffect
};

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
  terminator_type = 0;
}

ChartLoaderBMS::ChartLoaderBMS(Song *song)
  : ChartLoader(song), chart_context_(0), process_conditional_statement_(true)
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

bool ChartLoaderBMS::LoadFromDirectory()
{
  if (!song_->GetDirectory())
    return false;
  auto &dir = *song_->GetDirectory();

  if (dir.count() <= 0)
    return false;

  for (const auto *f : dir)
  {
    // first check filename, then attempt to read file.
    // (lazy-load for performance)
    const std::string filename = f->filename;
    if (!TestName(filename.c_str())) continue;
    if (!dir.Read(f->filename)) continue;

    Chart *c = song_->NewChart();
    if (!c) return false;

    bool r = Load(*c, f->p, f->len);
    c->SetFilename(filename);

    if (!r)
    {
      std::cerr << "Failed to read chart file (may be invalid) : " << filename << std::endl;
      song_->DeleteChart(song_->GetChartCount() - 1);
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
  Preload(c, p, iLen);
  ProcessCommand(c, static_cast<const char*>(p), iLen);
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

void ChartLoaderBMS::ProcessCommand(Chart &chart, const char* chr, int len)
{
  /** Initialize global context */
  memset(longnote_idx_per_lane, 0xffffffff, sizeof(longnote_idx_per_lane));
  cond_.clear();
  cond_.emplace_back(CondContext{ 0, -1, 0, 0, true });
  random_.SetSeed(seed_);
  bgm_column_idx_per_measure_.clear();
  // enable by default
  chart.GetTimingSegmentData().SetMeasureLengthRecover(true);
  chart_context_ = &chart;


  size_t pos = 0;
  size_t stmtlen = 0;
  size_t nextpos = 0;

  while (pos < len)
  {
    if (!IsCharacterTrimmable(chr[pos]))
    {
      stmtlen = 0;
      while (pos + stmtlen < len && chr[pos + stmtlen] != '\n')
        stmtlen++;
      nextpos = stmtlen + pos + 1;
      while (stmtlen > 0 && IsCharacterTrimmable(chr[pos + stmtlen - 1]))
        stmtlen--;

      // prepare for line
      current_line_ = new LineContext();
      current_line_->stmt = chr + pos;
      current_line_->stmt_len = stmtlen;
      if (!ParseCurrentLine())
      {
        delete current_line_;
        current_line_ = 0;
        pos = nextpos;
        continue;
      }

      // First check for conditional statement
      // Conditional statement is processed first and stored in FlushParsingBuffer.
      // If it's conditional statement and should not be processed,
      // It'll be stored in metadata area.
      if (IsCurrentLineIsConditionalStatement())
      {
        if (!process_conditional_statement_)
        {
          chart.GetMetaData().script +=
            std::string(current_line_->stmt, current_line_->stmt_len) + "\n";
        }
        else ParseControlFlow();
      }
      else if (cond_.back().parseable) /* parsable condition */
      {
        parsing_buffer_.push_back(current_line_);
      }

      current_line_ = 0;
      pos = nextpos;
    }
    else pos++;
  }

  /* Process parsed commands */
  FlushParsingBuffer();

  /* Sorting - TODO: not by row but measure */
  //chart.GetNoteData().SortByBeat();
}

void ChartLoaderBMS::ProcessConditionalStatement(bool do_process)
{
  process_conditional_statement_ = do_process;
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
  size_t len = current_line_->stmt_len;
  char *cw = current_line_->command;
  char terminator_type;
  c = p = current_line_->stmt;

  // check field separation message: ignored
  if (current_line_->stmt_len > 10 &&
      memcmp(current_line_->stmt, "*---------", 10) == 0)
    return false;

  // zero-size line: ignore
  if (current_line_->stmt_len == 0) return false;

  // check comment: ignored
  if (current_line_->stmt[0] == ';' ||
      (current_line_->stmt_len >= 2 && strncmp("//", current_line_->stmt, 2) == 0) )
    return false;

  // '%' start is ignored
  if (current_line_->stmt[0] == '%')
  {
    std::cerr << "percent starting line is ignored : "
              << std::string(current_line_->stmt, current_line_->stmt_len) << std::endl;
    return false;
  }

  // if not header field: ignored
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
    current_line_->value_len = 0;
    terminator_type = ' ';
  }
  else
  {
    current_line_->value_len = len-(c-p)-1;
    terminator_type = *c;
  }
  current_line_->value = c + 1;
  current_line_->terminator_type = terminator_type;

  return true;
}

#define CMDCMP(x) (strcmp(current_line_->command, (x)) == 0)

bool ChartLoaderBMS::IsCurrentLineIsConditionalStatement()
{
  return (CMDCMP("SWITCH") ||
    CMDCMP("RANDOM") || CMDCMP("RONDOM") || CMDCMP("SETRANDOM") ||
    CMDCMP("ENDRANDOM") || CMDCMP("ENDRONDOM") ||
    CMDCMP("IF") || CMDCMP("ELSE") || CMDCMP("ELSEIF") ||
    CMDCMP("ENDIF") || CMDCMP("END"));
}

bool ChartLoaderBMS::ParseControlFlow()
{
  unsigned int cond = atoi_bms_measure(current_line_->value, current_line_->value_len);

  if (CMDCMP("SWITCH"))
  {
    // check start of the stmt
    cond_.emplace_back(
      CondContext{ (int)cond, random_.Next(1, cond), 0, 0, true /* true for orphanized common part */ });
  }
  else if (CMDCMP("RANDOM") || CMDCMP("RONDOM") || CMDCMP("SETRANDOM"))
  {
    // check start of the stmt
    cond_.emplace_back(
      CondContext{ (int)cond, random_.Next(1, cond), 0, 0, true /* true for orphanized common part */ });
  }
  else if (CMDCMP("ENDRANDOM") || CMDCMP("ENDRONDOM"))
  {
    // restore chart context.
    cond_.pop_back();
  }
  else if (CMDCMP("IF"))
  {
    /**
     * COMMENT: IF statement can exist multiple times in same RANDOM block
     * So comment out assert below:
     * ASSERT(condstmt_->GetSentenceCount() == 0);
     */
     // JUST warning: no statement after ELSE
    if (cond_.back().condprocessed < 0)
      std::cerr << "BMSParser : IF clause after ELSE statement." << std::endl;
    cond_.back().parseable = cond_.back().condcur == (int)cond;
    cond_.back().condprocessed++;
    cond_.back().condcheckedcount += cond_.back().parseable;
  }
  else if (CMDCMP("ELSE"))
  {
    cond_.back().parseable =
      (cond_.back().condcheckedcount == 0 && cond_.back().condprocessed > 0);
    cond_.back().condprocessed = -9999; /* else state */
  }
  else if (CMDCMP("ELSEIF"))
  {
    // JUST warning: no statement after ELSE
    if (cond_.back().condprocessed < 0)
      std::cerr << "BMSParser : IF clause after ELSE statement." << std::endl;
    // JUST warning: IF statement must placed before ELSEIF
    if (cond_.back().condprocessed == 0)
      std::cerr << "BMSParser : ELSEIF clause before IF statement." << std::endl;
    cond_.back().parseable = cond_.back().condcur == (int)cond;
    cond_.back().condprocessed++;
    cond_.back().condcheckedcount += cond_.back().parseable;
  }
  else if (CMDCMP("ENDIF") || CMDCMP("END"))
  {
    // JUST warning: ENDIF without IF clause.
    if (cond_.back().condprocessed == 0)
      std::cerr << "BMSParser : ENDIF clause without IF statement." << std::endl;
    // just clear(init) chart context (unparse-able state)
    cond_.back().parseable = false;
    cond_.back().condprocessed = 0;
    cond_.back().condcheckedcount = 0;
  }
  else return false;
  return true;
}
#undef CMDCMP

bool ChartLoaderBMS::ParseMetaData()
{
  // check contains value
  if (current_line_->value_len == 0)
  {
    std::cerr << "Warning: Command #" << current_line_->command << " has no value, ignored." << std::endl;
    return true;
  }

  // cannot parse between control flow stmt
  if (!chart_context_) return false;
  MetaData& md = chart_context_->GetMetaData();
  std::string cmd(current_line_->command);
  std::string value(current_line_->value, current_line_->value_len);

  /**
   * @warn
   * LNTYPE metadata effects directly in syntax progress!
   */
  if (cmd == "LNTYPE")
    md.bms_longnote_type = atoi(value.c_str());
  else if (cmd == "LNOBJ")
    md.bms_longnote_object = atoi(value.c_str());

# define CHKCMD(cmd_const, len) (strncmp(current_line_->command, cmd_const, len) == 0)
  if (CHKCMD("BMP",3)) {
    unsigned int key = atoi_bms_channel(current_line_->command + 3);
    md.GetBGAChannel()->bga[key] = { value, 0,0,0,0, 0,0,0,0 };
  }
  else if (CHKCMD("WAV",3)) {
    unsigned int key = atoi_bms_channel(current_line_->command + 3);
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
    unsigned int key = atoi_bms_channel(current_line_->command + 4);
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
  return (radix_16 >> 4 /* / 16 */) * 36 + (radix_16 & 0b1111 /* % 16 */);
}

BmsChannelTypes GetNoteTypeFromBmsChannel(unsigned int bms_channel)
{
  switch (bms_channel)
  {
  case 1:   // BGM
    return BmsChannelTypes::kBgm;
  case 3:   // BPM change
  case 8:   // BPM (exbpm)
  case 9:   // STOP
    return BmsChannelTypes::kTiming;
  case 4:   // BGA
  case 6:   // BGA poor
  case 7:   // BGA layered
  case 10:  // BGA layered 2
    return BmsChannelTypes::kBga;
  case 11:  // BGA opacity
  case 12:  // BGA opacity layer
  case 13:  // BGA opacity layer 2
  case 14:  // BGA opacity poor
    return BmsChannelTypes::kEffect;
  default:
    // 1P/2P visible note
    if (bms_channel >= radix_16_2_36(0x11) && bms_channel <= radix_16_2_36(0x19) ||
        bms_channel >= radix_16_2_36(0x21) && bms_channel <= radix_16_2_36(0x29))
      return BmsChannelTypes::kNote;
    // 1P/2P invisible note
    else if (bms_channel >= radix_16_2_36(0x31) && bms_channel <= radix_16_2_36(0x39) ||
             bms_channel >= radix_16_2_36(0x41) && bms_channel <= radix_16_2_36(0x49))
      return BmsChannelTypes::kNote;
    // Longnote
    else if (bms_channel >= radix_16_2_36(0x51) && bms_channel <= radix_16_2_36(0x59) ||
             bms_channel >= radix_16_2_36(0x61) && bms_channel <= radix_16_2_36(0x69))
      return BmsChannelTypes::kNote;
    // Mine
    else if (bms_channel >= radix_16_2_36(0xD1) && bms_channel <= radix_16_2_36(0xD9) ||
             bms_channel >= radix_16_2_36(0xE1) && bms_channel <= radix_16_2_36(0xE9))
      return BmsChannelTypes::kNote;
  }
  return BmsChannelTypes::kNone;
}

#if 0
/* @depreciated */
uint8_t GetNotePlayerFromBmsChannel(unsigned int bms_channel)
{
  if (bms_channel >= radix_16_2_36(0x21) && bms_channel <= radix_16_2_36(0x29) ||
      bms_channel >= radix_16_2_36(0x41) && bms_channel <= radix_16_2_36(0x49) ||
      bms_channel >= radix_16_2_36(0x61) && bms_channel <= radix_16_2_36(0x69) ||
      bms_channel >= radix_16_2_36(0xE1) && bms_channel <= radix_16_2_36(0xE9))
    return 1;   // 2P
  return 0;     // 1P
}
#endif

/* SC is 8th ch */
/* in DP: SC is 15, 16th ch */
/* XXX: pedal is 9th ch, but when it would be in progress? */
uint8_t GetNoteColFromBmsChannel(unsigned int bms_channel)
{
  if (bms_channel < radix_16_2_36(0x10)) return 0;  // special channel
  unsigned int channel_mod_16 = bms_channel % 36;
  if (channel_mod_16 >= 16) return 0;
  unsigned int channel_remap[16] = 
  {0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 0, 0, 0, 0, 0, 0};
  return static_cast<uint8_t>(channel_remap[channel_mod_16] - 1);
}

bool ChartLoaderBMS::ParseMeasureLength()
{
  std::string v(current_line_->value, current_line_->value_len);
  NoteElement ne;
  ne.SetRowPos(current_line_->measure, 1, 0);
  ne.set_value((double)atof(v.c_str()));
  chart_context_->GetTimingData()[TimingTrackTypes::kMeasure].AddNoteElement(ne);
  return true;
}

bool ChartLoaderBMS::ParseNote()
{
  const unsigned int channel = current_line_->bms_channel;
  unsigned int len = current_line_->value_len;

  // cannot parse between control flow stmt
  if (!chart_context_) return false;

  // check for measure length
  // - if measure changed, 
  if (channel == 2)
    return ParseMeasureLength();

  // warn for incorrect length
  if (len % 2 == 1)
  {
    std::cerr << "Warning: incorrect measure length detected. (" << current_line_->command << ")" << std::endl;
    len--;
  }
  if (len == 0) return false;


  curr_note_syntax_.measure = current_line_->measure;
  curr_note_syntax_.channel = current_line_->bms_channel;
  curr_note_syntax_.deno = current_line_->value_len;

  for (unsigned int i = 0; i < len; i += 2)
  {
    curr_note_syntax_.num = i;
    curr_note_syntax_.value = current_line_->value + i;
    curr_note_syntax_.value_u = atoi_bms_channel(curr_note_syntax_.value);

    switch (GetNoteTypeFromBmsChannel(current_line_->bms_channel))
    {
    case BmsChannelTypes::kBgm:
      ParseBgmNote();
      break;
    case BmsChannelTypes::kBga:
      ParseBgaNote();
      break;
    case BmsChannelTypes::kNote:
      ParseSoundNote();
      break;
    case BmsChannelTypes::kTiming:
      ParseTimingNote();
      break;
    case BmsChannelTypes::kEffect:
      ParseEffectNote();
      break;
    }
  }

  switch (GetNoteTypeFromBmsChannel(current_line_->bms_channel))
  {
  case BmsChannelTypes::kBgm:
    // if bgm channel, then add track idx.
    bgm_column_idx_per_measure_[current_line_->measure]++;
    break;
  }

  return true;
}

bool ChartLoaderBMS::ParseBgaNote()
{
  if (curr_note_syntax_.value_u == 0) return true;
  unsigned track_idx = 0;
  NoteElement ne;
  ne.SetRowPos(curr_note_syntax_.measure, curr_note_syntax_.deno, curr_note_syntax_.num);
  ne.set_value((int)curr_note_syntax_.value_u);
  switch (curr_note_syntax_.channel)
  {
  default:  // XXX: exception when undefined?
  case 4:   // BGA
    track_idx = CommandTrackTypes::kBgaMain;
    break;
  case 6:   // BGA poor
    track_idx = CommandTrackTypes::kBgaMiss;
    break;
  case 7:   // BGA layered
    track_idx = CommandTrackTypes::kBgaLayer1;
    break;
  case 10:  // BGA layered 2
    track_idx = CommandTrackTypes::kBgaLayer2;
    break;
  }

  chart_context_->GetCommandData()[track_idx].AddNoteElement(ne);

  return true;
}

bool ChartLoaderBMS::ParseBgmNote()
{
  if (curr_note_syntax_.value_u == 0) return true;
  unsigned track = bgm_column_idx_per_measure_[current_line_->measure];
  NoteElement ne;

  ne.SetRowPos(curr_note_syntax_.measure, curr_note_syntax_.deno, curr_note_syntax_.num);
  ne.set_value((int)curr_note_syntax_.value_u);

  chart_context_->GetBgmData()[track].AddNoteElement(ne);

  return true;
}

bool ChartLoaderBMS::ParseEffectNote()
{
  if (curr_note_syntax_.value_u == 0) return true;
  NoteElement ne;
  unsigned channel = curr_note_syntax_.channel;
  unsigned track = 0;
  const int bmsargb_per_bms_channel[] = { 11, 12, 13, 14 };

  ne.SetRowPos(curr_note_syntax_.measure, curr_note_syntax_.deno, curr_note_syntax_.num);

  switch (channel)
  {
  case 11:  // BGA opacity
  case 12:  // BGA opacity layer
  case 13:  // BGA opacity layer 2
  case 14:  // BGA opacity poor
    track = CommandTrackTypes::kBmsARGBLAYER;
    break;
  }

  switch (track)
  {
  case CommandTrackTypes::kBmsARGBLAYER:
    {
      int bgatypes;
      for (bgatypes = 0; bgatypes < 4; bgatypes++)
        if (bmsargb_per_bms_channel[bgatypes] == channel) break;
      ASSERT(bgatypes < 4);
      ne.set_point(bgatypes);
      ne.set_value(curr_note_syntax_.value_u);
    }
    break;
  default:
    ASSERT(0);
  }

  chart_context_->GetCommandData()[track].AddNoteElement(ne);

  return true;
}

inline int GetNoteSubTypeFromBmsChannel(unsigned int bms_channel, bool &is_longnote)
{
  // 1P/2P visible note
  if (bms_channel >= radix_16_2_36(0x11) && bms_channel <= radix_16_2_36(0x19) ||
    bms_channel >= radix_16_2_36(0x21) && bms_channel <= radix_16_2_36(0x29))
    return NoteTypes::kNormalNote;
  // 1P/2P invisible note
  else if (bms_channel >= radix_16_2_36(0x31) && bms_channel <= radix_16_2_36(0x39) ||
    bms_channel >= radix_16_2_36(0x41) && bms_channel <= radix_16_2_36(0x49))
    return NoteTypes::kInvisibleNote;
  // Longnote
  else if (bms_channel >= radix_16_2_36(0x51) && bms_channel <= radix_16_2_36(0x59) ||
    bms_channel >= radix_16_2_36(0x61) && bms_channel <= radix_16_2_36(0x69))
  {
    is_longnote = true;
    return NoteTypes::kNormalNote;
  }
  // Mine
  else if (bms_channel >= radix_16_2_36(0xD1) && bms_channel <= radix_16_2_36(0xD9) ||
    bms_channel >= radix_16_2_36(0xE1) && bms_channel <= radix_16_2_36(0xE9))
    return NoteTypes::kMineNote;

  // Should not reach here. (strictly)
  //ASSERT(0);
  // or, regard as general note.
  return NoteTypes::kNormalNote;
}

bool ChartLoaderBMS::ParseSoundNote()
{
  const unsigned measure = curr_note_syntax_.measure;
  const unsigned channel = curr_note_syntax_.channel;
  const char* val = curr_note_syntax_.value;
  const unsigned valu = curr_note_syntax_.value_u;
  const int longnotetype = chart_context_->GetMetaData().bms_longnote_type;
  const unsigned lnobj = (unsigned)chart_context_->GetMetaData().bms_longnote_object;
  bool is_longnote = false;
  const unsigned subtype = GetNoteSubTypeFromBmsChannel(channel, is_longnote);
  unsigned curlane = 0;
  NoteElement ne;
  curlane = GetNoteColFromBmsChannel(channel);
  auto &track = chart_context_->GetNoteData()[curlane];

  if (valu == 0)
  {
    /** Close longnote in case of LNTYPE 2 (obsolete) */
    if (longnotetype == 2)
      longnote_idx_per_lane[curlane] = UINT32_MAX;
    /** Don't do anything with 00 note */
    return true;
  }

  ne.SetRowPos(curr_note_syntax_.measure, curr_note_syntax_.deno, curr_note_syntax_.num);
  ne.set_value(valu);

  /** Longnote check */
  if (is_longnote || (valu == lnobj) /* LNOBJ check */ )
  {
    /** LNTYPE 2 (obsolete) */
    if (longnotetype == 2)
    {
      if (longnote_idx_per_lane[curlane] == UINT32_MAX)
      {
        longnote_idx_per_lane[curlane] = 1;
        track.AddNoteElement(ne); /* begin */
        ne.set_chain_status(NoteChainStatus::End);
        track.AddNoteElement(ne); /* end */
      }
      else
      {
        /* Don't add note, only change ending position. */
        track.back().SetRowPos(curr_note_syntax_.measure, curr_note_syntax_.deno, curr_note_syntax_.num);
      }
    }
    /** LNTYPE 1 (default) or LNOBJ */
    else
    {
      if (longnote_idx_per_lane[curlane] == UINT32_MAX)
      {
        longnote_idx_per_lane[curlane] = 1;
        track.AddNoteElement(ne); /* begin */
      }
      else
      {
        ne.set_chain_status(NoteChainStatus::End);
        track.AddNoteElement(ne); /* end */
        return true; /* Don't add note */
      }
    }
  }

  return true;
}

bool ChartLoaderBMS::ParseTimingNote()
{
  if (curr_note_syntax_.value_u == 0) return true;

  NoteElement ne;
  unsigned channel = curr_note_syntax_.channel;
  unsigned track = 0;
  ne.SetRowPos(curr_note_syntax_.measure, curr_note_syntax_.deno, curr_note_syntax_.num);

  switch (channel)
  {
  case 3:   // BPM change
    track = TimingTrackTypes::kBpm;
    /** 16 radix here */
    ne.set_value((unsigned)atoi_16(curr_note_syntax_.value, 2));
    break;
  case 8:   // BPM (exbpm)
    track = TimingTrackTypes::kBmsBpm;
    ne.set_value(curr_note_syntax_.value_u);
    break;
  case 9:   // STOP
    track = TimingTrackTypes::kBmsStop;
    ne.set_value(curr_note_syntax_.value_u);
    break;
  default:
    ASSERT(0);
  }

  chart_context_->GetTimingData()[track].AddNoteElement(ne);
  return true;
}

void ChartLoaderBMS::FlushParsingBuffer()
{
  char terminator_type;
  for (auto &ii : parsing_buffer_)
  {
    current_line_ = ii;
    terminator_type = current_line_->terminator_type;

    if (terminator_type == ':')
    {
      current_line_->measure = atoi_bms_measure(current_line_->command, 3);
      current_line_->bms_channel = atoi_bms_channel(current_line_->command + 3, 2);
      if (!ParseNote())
        std::cerr << "BMSParser: note parsing failed " << std::string(current_line_->stmt, current_line_->stmt_len) << std::endl;
    }
    else if (terminator_type == ' ')
    {
      if (!ParseMetaData())
        std::cerr << "BMSParser: meta parsing failed " << std::string(current_line_->stmt, current_line_->stmt_len) << std::endl;
    }
    else
    {
      std::cerr << "BMSParser: unknown terminator " << std::string(current_line_->stmt, current_line_->stmt_len) << std::endl;
    }
  }

  for (auto &ii : parsing_buffer_)
    delete ii;
  parsing_buffer_.clear();

  cond_.clear();
}

} /* rparser */
