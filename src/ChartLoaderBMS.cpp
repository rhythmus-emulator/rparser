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

// main file
bool ChartLoaderBMS::Test( const void* p, int iLen )
{
  // As bms file has no file signature,
  // just search for "*--" string for first 100 string.
  // TODO: is there any exception?
  for (int i=0; i<iLen-4 && i<100; i++) {
    if (strncmp((const char*)p, "*-----", 6)==0)
      return true;
  }
  return false;
}

bool TestName( const char *fn )
{
  std::string fn_lower = fn;  lower(fn_lower);
  return endsWith(fn_lower, ".bms") ||
         endsWith(fn_lower, ".bme") ||
         endsWith(fn_lower, ".bml") ||
         endsWith(fn_lower, ".pms");
}

bool ChartLoaderBMS::Load( const void* p, int iLen )
{
  const char *chr = static_cast<const char*>(p);
  const char *linestart = static_cast<const char*>(p);
  do
  {
    if (*chr == '\n')
    {
      const char *st_trimmed = linestart;
      const char *chr_trimmed = chr;
      while (chr_trimmed > st_trimmed && 
             (*st_trimmed == ' ' || *st_trimmed == '\t' || *st_trimmed == '\r'))
        st_trimmed++;
      while (chr_trimmed > st_trimmed && 
             (*chr_trimmed == ' ' || *chr_trimmed == '\t' || *chr_trimmed == '\r'))
        chr_trimmed--;
      current_line_.clear();
      current_line_.stmt = chr_trimmed;
      current_line_.stmt_len = chr_trimmed-st_trimmed;
      if (!ParseCurrentLine())
      {
        // XXX: for debug
        std::string de(current_line_.stmt, current_line_.stmt_len);
        printf("parse failed: \"%s\"", de.c_str());
      }
      chr = linestart+1;
    }
    chr++;
  } while (*chr);

  return true;
}

bool ChartLoaderBMS::TestName(const char *fn)
{
  return endsWith(fn, ".bms", false) ||
    endsWith(fn, ".bme", false) ||
    endsWith(fn, ".bml", false);
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
  while (p && length)
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
  const unsigned int len = current_line_.stmt_len;
  char *cw = current_line_.command;
  char terminator_type;
  c = p = current_line_.stmt;

  if (*c != '#') return false;
  while (*c != ' ' || *c != ':')
  {
    if (c-p > (int)len || c-p > 256) break;
    *cw = upperchr(*c);
    c++; cw++;
  }
  *cw = 0;
  terminator_type = *c;
  current_line_.value_len = len+p-c-1;

  if (terminator_type == ':')
  {
    if (!ParseControlFlow() && !ParseMetaData())
      return false;
  }
  else if (terminator_type == ' ')
  {
    current_line_.measure = atoi_bms_measure(current_line_.command, 3);
    current_line_.bms_channel = atoi_bms_channel(current_line_.command+3, 2);
    if (!ParseNote())
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
    ASSERT(condstmt_->GetSentenceCount() == 0);
    chart_context_ = condstmt_->CreateSentence(cond);
  }
  else if (CMDCMP("ELSE"))
  {
    ASSERT(condstmt_->GetSentenceCount() > 0);
    chart_context_ = condstmt_->CreateSentence(-1);
  }
  else if (CMDCMP("ELSEIF"))
  {
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

#define CMDCMP(x) (cmd == (x))
bool ChartLoaderBMS::ParseMetaData()
{
  // cannot parse between control flow stmt
  if (!chart_context_) return false;
  MetaData& md = chart_context_->GetMetaData();
  std::string cmd(current_line_.command);
  std::string value(current_line_.value, current_line_.value_len);

  if (CMDCMP("TITLE")) {
    std::swap(md.title, value);
  }
  else if (CMDCMP("SUBTITLE")) {
    std::swap(md.subtitle, value);
  }
  else if (CMDCMP("ARTIST")) {
    std::swap(md.artist, value);
  }
  else if (CMDCMP("SUBARTIST")) {
    std::swap(md.subartist, value);
  }
  else if (CMDCMP("GENRE")) {
    std::swap(md.genre, value);
  }
  else if (CMDCMP("PLAYER")) {
    int pl = atoi(value.c_str());
    if (pl == 2 || pl == 4)
      md.player_count = 2;
    else
      md.player_count = 1;
    md.player_side = 1;
  }
  else if (CMDCMP("PLAYLEVEL")) {
    md.level = atoi(value.c_str());
  }
  else if (CMDCMP("DIFFICULTY")) {
    md.difficulty = atoi(value.c_str());
  }
  else if (CMDCMP("RANK")) {
    // convert from 4 to 100.0
    md.judge_timing = atof(value.c_str()) / 4.0 * 100;
  }
  else if (CMDCMP("TOTAL")) {
    md.gauge_total = atof(value.c_str());
  }
  else if (CMDCMP("BANNER")) {
    md.banner_image = value;
  }
  else if (CMDCMP("BACKBMP")) {
    md.back_image = value;
  }
  else if (CMDCMP("STAGEFILE")) {
    md.stage_image = value;
  }
  else if (CMDCMP("BPM")) {
    md.bpm = atof(value.c_str());
  }
  else if (CMDCMP("LNTYPE")) {
    md.bms_longnote_type = atoi(value.c_str());
  }
  else if (CMDCMP("LNOBJ")) {
    md.bms_longnote_object = atoi_bms_channel(value.c_str());
  }
  else if (CMDCMP("MUSIC")) {
    md.background_music = value;
  }
  else if (CMDCMP("PREVIEW")) {
    md.preview_music = value;
  }
  else if (CMDCMP("OFFSET")) {
    md.time_0beat_offset = atof(value.c_str());
  }

  #define CHKCMD(cmd_const, len) (strncmp(current_line_.command, cmd_const, len) == 0)
  else if (CHKCMD("BMP",3)) {
    unsigned int key = atoi_bms_channel(current_line_.value + 3);
    md.GetBGAChannel()->bga[key] = { value, 0,0,0,0, 0,0,0,0 };
  }
  else if (CHKCMD("WAV",3)) {
    unsigned int key = atoi_bms_channel(current_line_.value + 3);
    md.GetSoundChannel()->fn[key] = value;
  }
  else if (CHKCMD("EXBPM",5) || CHKCMD("BPM",3)) {
    unsigned int key;
    if (current_line_.value[0] == 'B')
        key = atoi_bms_channel(current_line_.value + 3);
    else
        key = atoi_bms_channel(current_line_.value + 5);
    md.GetBPMChannel()->bpm[key] = static_cast<float>(atof( value.c_str() ));
  }
  else if (CHKCMD("STOP",4)) {
    unsigned int key = atoi_bms_channel(current_line_.value  + 4);
    md.GetSTOPChannel()->stop[key] = static_cast<float>(atoi(value.c_str()));  // 1/192nd
  }
  else if (value == "#stp") {
    std::string sMeasure, sTime;
    if (!split(value, ' ', sMeasure, sTime))
    {
      printf("invalid #STP found, ignore. (%s)\n", value.c_str());
      return false;
    }
    float fMeasure = static_cast<float>(atof(sMeasure.c_str()));
    md.GetSTOPChannel()->STP[fMeasure] = static_cast<float>(atoi(sTime.c_str()));
  }
  // TODO: WAVCMD, EXWAV
  // TODO: MIDIFILE
  #undef CHKCMD

  return true;
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
    return NoteTypes::kBGA;
  case 11:  // BGA opacity
  case 12:  // BGA opacity layer
  case 13:  // BGA opacity layer 2
  case 14:  // BGA opacity poor
    return NoteTypes::kSpecial;
  default:
    // 1P/2P visible note
    if (bms_channel >= 0x11 && bms_channel <= 0x19 ||
        bms_channel >= 0x21 && bms_channel <= 0x29)
      return NoteTypes::kNote;
    // 1P/2P invisible note
    else if (bms_channel >= 0x31 && bms_channel <= 0x39 ||
             bms_channel >= 0x41 && bms_channel <= 0x49)
      return NoteTypes::kNote;
    // Longnote
    else if (bms_channel >= 0x51 && bms_channel <= 0x59 ||
             bms_channel >= 0x61 && bms_channel <= 0x69)
      return NoteTypes::kNote;
    // Mine
    else if (bms_channel >= 0xD1 && bms_channel <= 0xD9 ||
             bms_channel >= 0xE1 && bms_channel <= 0xE9)
      return NoteTypes::kNote;
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
    return NoteBgaTypes::kMainBga;
  case 6:   // BGA poor
    return NoteBgaTypes::kMissBga;
  case 7:   // BGA layered
    return NoteBgaTypes::kLAYER1Bga;
  case 10:  // BGA layered 2
    return NoteBgaTypes::kLAYER2Bga;
  case 11:  // BGA opacity
    return NoteSpecialTypes::kBmsARGBBASE;
  case 12:  // BGA opacity layer
    return NoteSpecialTypes::kBmsARGBLAYER1;
  case 13:  // BGA opacity layer 2
    return NoteSpecialTypes::kBmsARGBLAYER2;
  case 14:  // BGA opacity poor
    return NoteSpecialTypes::kBmsARGBMISS;
  default:
    // 1P/2P visible note
    if (bms_channel >= 0x11 && bms_channel <= 0x19 ||
        bms_channel >= 0x21 && bms_channel <= 0x29)
      return NoteSubTypes::kNormalNote;
    // 1P/2P invisible note
    else if (bms_channel >= 0x31 && bms_channel <= 0x39 ||
             bms_channel >= 0x41 && bms_channel <= 0x49)
      return NoteSubTypes::kInvisibleNote;
    // Longnote
    else if (bms_channel >= 0x51 && bms_channel <= 0x59 ||
             bms_channel >= 0x61 && bms_channel <= 0x69)
      return NoteSubTypes::kLongNote;
    // Mine
    else if (bms_channel >= 0xD1 && bms_channel <= 0xD9 ||
             bms_channel >= 0xE1 && bms_channel <= 0xE9)
      return NoteSubTypes::kMineNote;
  }
  return 0;
}

uint8_t GetNotePlayerFromBmsChannel(unsigned int bms_channel)
{
  if (bms_channel >= 0x21 && bms_channel <= 0x29 ||
      bms_channel >= 0x41 && bms_channel <= 0x49 ||
      bms_channel >= 0x61 && bms_channel <= 0x69 ||
      bms_channel >= 0xE1 && bms_channel <= 0xE9)
    return 1;   // 2P
  return 0;
}

/* SC is 8th ch */
/* in DP: SC is 15, 16th ch */
/* XXX: pedal is 9th ch, but when it would be in progress? */
uint8_t GetNoteColFromBmsChannel(unsigned int bms_channel)
{
  if (bms_channel < 0x20) return 0;
  unsigned int channel_mod_16 = bms_channel % 16;
  unsigned int channel_remap[16] = 
  {0, 1, 2, 3, 4, 5, 8, 9, 6, 7, 0, 0, 0, 0, 0, 0};
  return channel_remap[channel_mod_16];
}

bool ChartLoaderBMS::ParseNote()
{
  unsigned int value_u;
  SoundNote n;
  const unsigned int measure = current_line_.measure;
  const unsigned int channel = current_line_.bms_channel;
  const char* value = current_line_.value;
  unsigned int len = current_line_.value_len;

  // cannot parse between control flow stmt
  if (!chart_context_) return false;

  // warn for incorrect length
  if (len % 2 == 1)
  {
    printf("Warning: incorrect measure length detected.\n");
    len --;
  }
  if (len == 0) return false;

  memset(&n, 0, sizeof(Note));
  NotePos& npos = n.GetNotePos();
  npos.type = NotePosTypes::Row;
  npos.row.measure = measure;
  npos.row.deno = len;
  n.SetNotetype(GetNoteTypeFromBmsChannel(channel));
  n.SetNoteSubtype(GetNoteSubTypeFromBmsChannel(channel));
  n.track.lane.note.player = GetNotePlayerFromBmsChannel(channel);
  n.track.lane.note.lane = GetNoteColFromBmsChannel(channel);
  for (unsigned int i = 0; i < len; i += 2)
  {
    value_u = atoi_bms_channel(value+i);
    if (value_u == 0) continue;
    npos.row.num = i;
    n.value = static_cast<Channel>(value_u);
    // TODO: do process for LNTYPE1/2 chaining.
    chart_context_->GetNoteData().AddNote(n);
  }

  return true;
}

} /* rparser */
