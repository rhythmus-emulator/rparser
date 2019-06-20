#include "ChartUtil.h"
#include "Song.h"

/*
 * XXX: rand() method could be distrubed by other program/threads seed reseting.
 *      We may need to make rand method with its own context.
 */

namespace rparser
{

std::vector<Note*>& NoteSelection::GetSelection()
{
	return m_vNotes;
}

std::vector<Note*>::iterator NoteSelection::begin()
{
	return m_vNotes.begin();
}

std::vector<Note*>::iterator NoteSelection::end()
{
	return m_vNotes.end();
}

void NoteSelection::Clear()
{
	m_vNotes.clear();
}

class HTMLExporter
{
public:
  HTMLExporter();
  void PushIndent();
  void PopIndent();
  std::stringstream& line();
private:
  unsigned indentlevel_;
  std::stringstream ss_;
};

HTMLExporter::HTMLExporter()
  : indentlevel_(0) {}

void HTMLExporter::PushIndent() { indentlevel_++; }
void HTMLExporter::PopIndent() { if (indentlevel_ > 0) indentlevel_--; }
std::stringstream& HTMLExporter::line()
{
  ss_ << std::endl;
  for (unsigned i = 0; i < indentlevel_; ++i)
    ss_ << "\t";
  return ss_;
}

void ExportToHTML(const Chart &c, std::string& out)
{
  HTMLExporter e;

  auto &md = c.GetMetaData();
  auto &nd = c.GetNoteData();
  auto &ed = c.GetEventNoteData();
  auto &td = c.GetTempoData();
  auto &tnd = td.GetTempoNoteData();

  // STEP 0. Container
  e.line() << "<div id='rhythmus-container' class='playtype-" << GetExtensionBySongType(c.GetSongType()) <<
    " playlane-" << (int)c.GetPlayLaneCount() << "key'>";
  e.PushIndent();

  // STEP 1. Metadata
  {
    e.line() << "<div id='metadata' class='content metadata'>";
    e.PushIndent();
    e.line() << "<span class='title'>Metadata Info</span>";
    // meta related with song
    e.line() << "<span class='desc meta_title'><span class='label'>Filename</span><span class='text'>" << c.GetFilename() << "</span></span>";
    e.line() << "<span class='desc meta_filetype'><span class='label'>Filetype</span><span class='text'>" << GetExtensionBySongType(c.GetSongType()) << "</span></span>";
    e.line() << "<span class='desc meta_playmode'><span class='label'>PlayMode</span><span class='text'>" << (int)c.GetPlayLaneCount() << "Key</span></span>";
    // meta related with metadata
    e.line() << "<span class='desc meta_title'><span class='label'>Title</span><span class='text'>" << md.title <<
      "<span class='meta_subtitle'>" << md.subtitle << "</span>" << "</span></span>";
    e.line() << "<span class='desc meta_artist'><span class='label'>Artist</span><span class='text'>" << md.artist <<
      "<span class='meta_subartist'>" << md.subartist << "</span>" << "</span></span>";
    e.line() << "<span class='desc meta_level'><span class='label'>Level</span><span class='text'>" << md.level << "</span></span>";
    e.line() << "<span class='desc meta_bpm'><span class='label'>BPM</span><span class='text'>" << md.bpm << "</span></span>";
    e.line() << "<span class='desc meta_total'><span class='label'>Gauge Total</span><span class='text'>" << md.gauge_total << "</span></span>";
    e.line() << "<span class='desc meta_diff'><span class='label'>Difficulty</span><span class='text'>" << md.difficulty << "</span></span>";
    // meta related with notedata
    e.line() << "<span class='desc meta_notecount'><span class='label'>Note Count</span><span class='text'>" << c.GetScoreableNoteCount() << "</span></span>";
    // meta related with eventdata
    e.line() << "<span class='desc meta_eventcount'><span class='label'>Event Count</span><span class='text'>" << ed.size() << "</span></span>";
    // meta related with tempodata
    e.line() << "<span class='desc meta_maxbpm'><span class='label'>Max BPM</span><span class='text'>" << td.GetMaxBpm() << "</span></span>";
    e.line() << "<span class='desc meta_minbpm'><span class='label'>Min BPM</span><span class='text'>" << td.GetMinBpm() << "</span></span>";
    e.line() << "<span class='desc meta_isbpmchange'><span class='label'>BPM Change?</span><span class='text'>" << (td.HasBpmChange() ? "Yes" : "No") << "</span></span>";
    e.line() << "<span class='desc meta_hasstop'><span class='label'>BPM Change?</span><span class='text'>" << (td.HasStop() ? "Yes" : "No") << "</span></span>";
    e.line() << "<span class='desc meta_haswarp'><span class='label'>STOP?</span><span class='text'>" << (td.HasWarp() ? "Yes" : "No") << "</span></span>";
    e.line() << "<span class='desc meta_haswarp'><span class='label'>WARP?</span><span class='text'>" << (td.HasWarp() ? "Yes" : "No") << "</span></span>";
    uint32_t lasttime = static_cast<uint32_t>(c.GetSongLastObjectTime());
    char lasttime_str[3][3];
    sprintf(lasttime_str[0], "%02d", lasttime / 3600000);
    sprintf(lasttime_str[1], "%02d", lasttime / 60000 % 60);
    sprintf(lasttime_str[2], "%02d", lasttime / 1000 % 60);
    e.line() << "<span class='desc meta_songlength'><span class='label'>Song Length</span><span class='text'>" <<
      lasttime_str[0] << ":" << lasttime_str[1] << ":" << lasttime_str[2] << "</span></span>";
    e.PopIndent();
    e.line() << "</div>";
  }

  // STEP 2. Render objects
  {
    e.line() << "<div id='notedata' class='content notedata'>";
    e.PushIndent();

    // each measure has each div box
    uint32_t measure_idx = 1;
    uint32_t nd_idx = 0, ed_idx = 0, td_idx = 0;
    while (nd_idx < nd.size() || ed_idx < ed.size() || td_idx < tnd.size())
    {
      double beat = td.GetBeatFromRow(measure_idx);

      // measure start.
      e.line() << "<div id='measure" << measure_idx << "' class='measurebox'" <<
        " data-length=" << td.GetMeasureFromBeat(beat) << " data-beat=" << beat << ">";
      e.PushIndent();
      e.line() << "<div class='measureno'>" << measure_idx << "</div>";

      // STEP 2-1. NoteData
      while (nd_idx < nd.size() && nd.get(nd_idx).pos().measure < measure_idx)
      {
        /* XXX: don't use continue here; MUST go through full loop. */
        auto &n = nd.get(nd_idx);
        if (n.type() == NoteTypes::kTap)
        {
          double ypos = (n.pos().measure - (int)n.pos().measure) * 100;
          e.line() << "<div id='nd" << nd_idx << "' class='chartobject noteobject lane" << (int)n.GetLane() <<
            "' style='top:" << (int)ypos << "%'" << /* style end */
            " data-x=" << (int)n.GetLane() << " data-y=" << (int)ypos << " data-beat=" << n.beat << /* data end */
            "></div>";
        }
        nd_idx++;
      } /* TODO: LONGNOTE */
      // STEP 2-2. TempoData
      while (td_idx < tnd.size() && tnd.get(td_idx).pos().measure < measure_idx)
      {
        auto &n = tnd.get(td_idx);
        double ypos = (n.pos().measure - (int)n.pos().measure) * 100;
        e.line() << "<div id='td" << td_idx << "' class='chartobject tempoobject" <<
          "' style='top:" << (int)ypos << "%'" << /* style end */
          " data-y=" << (int)ypos << " data-beat=" << n.beat << /* data end */
          "></div>";
        td_idx++;
      }
      // STEP 2-3. EventData
      while (ed_idx < ed.size() && ed.get(ed_idx).pos().measure < measure_idx)
      {
        auto &n = ed.get(ed_idx);
        double ypos = (n.pos().measure - (int)n.pos().measure) * 100;
        e.line() << "<div id='ed" << ed_idx << "' class='chartobject eventobject" <<
          "' style='top:" << (int)ypos << "%'" << /* style end */
          " data-y=" << (int)ypos << " data-beat=" << n.beat << /* data end */
          "></div>";
        ed_idx++;
      }

      // measure end.
      e.PopIndent();
      e.line() << "</div>";
      measure_idx++;
    }

    e.PopIndent();
    e.line() << "</div>";
  }

  e.PopIndent();
  e.line() << "</div>";

  out = e.line().str();
}

void ClipRange(Chart &c, double beat_start, double beat_end)
{
  // TODO
  ASSERT(0);
}

namespace effector
{

void SetLanefor7Key(EffectorParam& param)
{
  param.player = 0;
  param.lanesize = 7;
  memset(param.lockedlane, 0, sizeof(param.lockedlane));
  param.lockedlane[0] = NOTE;
  param.lockedlane[1] = NOTE;
  param.lockedlane[2] = NOTE;
  param.lockedlane[3] = NOTE;
  param.lockedlane[4] = NOTE;
  param.lockedlane[5] = NOTE;
  param.lockedlane[6] = NOTE;
}

void SetLanefor9Key(EffectorParam& param)
{
  param.player = 0;
  param.lanesize = 9;
  memset(param.lockedlane, 0, sizeof(param.lockedlane));
  param.lockedlane[0] = NOTE;
  param.lockedlane[1] = NOTE;
  param.lockedlane[2] = NOTE;
  param.lockedlane[3] = NOTE;
  param.lockedlane[4] = NOTE;
  param.lockedlane[5] = NOTE;
  param.lockedlane[6] = NOTE;
  param.lockedlane[7] = NOTE;
  param.lockedlane[8] = NOTE;
}

void SetLaneforBMS1P(EffectorParam& param)
{
  param.player = 0;
  param.lanesize = 8;
  memset(param.lockedlane, 0, sizeof(param.lockedlane));
  param.lockedlane[0] = NOTE;
  param.lockedlane[1] = NOTE;
  param.lockedlane[2] = NOTE;
  param.lockedlane[3] = NOTE;
  param.lockedlane[4] = NOTE;
  param.lockedlane[5] = NOTE;
  param.lockedlane[6] = NOTE;
  param.lockedlane[7] = SC;
}

void SetLaneforBMS2P(EffectorParam& param)
{
  param.player = 1;
  param.lanesize = 8;
  memset(param.lockedlane, 0, sizeof(param.lockedlane));
  param.lockedlane[0] = NOTE;
  param.lockedlane[1] = NOTE;
  param.lockedlane[2] = NOTE;
  param.lockedlane[3] = NOTE;
  param.lockedlane[4] = NOTE;
  param.lockedlane[5] = NOTE;
  param.lockedlane[6] = NOTE;
  param.lockedlane[7] = SC;
}

void SetLaneforBMSDP1P(EffectorParam& param)
{
  param.player = 0;
  param.lanesize = 16;
  memset(param.lockedlane, 0, sizeof(param.lockedlane));
  param.lockedlane[0] = NOTE;
  param.lockedlane[1] = NOTE;
  param.lockedlane[2] = NOTE;
  param.lockedlane[3] = NOTE;
  param.lockedlane[4] = NOTE;
  param.lockedlane[5] = NOTE;
  param.lockedlane[6] = NOTE;
  param.lockedlane[14] = SC;
  param.lockedlane[15] = SC;
}

void SetLaneforBMSDP2P(EffectorParam& param)
{
  param.player = 0;
  param.lanesize = 16;
  memset(param.lockedlane, 0, sizeof(param.lockedlane));
  param.lockedlane[7] = NOTE;
  param.lockedlane[8] = NOTE;
  param.lockedlane[9] = NOTE;
  param.lockedlane[10] = NOTE;
  param.lockedlane[11] = NOTE;
  param.lockedlane[12] = NOTE;
  param.lockedlane[13] = NOTE;
  param.lockedlane[14] = SC;
  param.lockedlane[15] = SC;
}

void GenerateRandomColumn(int *new_col, const EffectorParam& param)
{
  ASSERT(param.lanesize < kMaxSizeLane);
  int lanes_to_randomize = param.lanesize;
  for (int i=0; i<param.lanesize; i++)
  {
    // if locked lane then stay calm
    if (param.lockedlane[i] == LaneTypes::LOCKED)
    {
      new_col[i] = i;
    }
    else lanes_to_randomize--;
  }
  for (int i=0, lockedcnt=0; i<param.lanesize && lanes_to_randomize; i++)
  {
    // randomly set free lanes
    if (param.lockedlane[i] == LaneTypes::NOTE)
    {
      new_col[i] = rand() % lanes_to_randomize + lockedcnt;
      lockedcnt++;
      lanes_to_randomize--;
    }
  }
  ASSERT(lanes_to_randomize == 0);
}

inline bool CheckNoteValidity(SoundNote& note, const EffectorParam& param)
{
  int current_col = note.track.lane.note.lane;
  int current_player = note.track.lane.note.player;
  if (note.type() != NoteTypes::kTap)
    return false;
  if (current_player != param.player)
    return false;
  ASSERT(note.track.lane.note.lane < kMaxSizeLane);
  return true;
}

void Random(Chart &c, const EffectorParam& param)
{
  int new_col[kMaxSizeLane];

  GenerateRandomColumn(new_col, param);

  c.GetNoteData().SortByBeat();

  for (auto& note: c.GetNoteData())
  {
    if (!CheckNoteValidity(note, param)) continue;
    note.track.lane.note.lane = new_col[note.track.lane.note.lane];
  }
}

void SRandom(Chart &c, const EffectorParam& param)
{
  // SRANDOM algorithm from
  // https://gall.dcinside.com/mgallery/board/view/?id=popn&no=3291&page=1484
  Random(c, param);
  RRandom(c, param, true);
  Random(c, param);
}

void HRandom(Chart &c, const EffectorParam& param)
{
  int new_col[kMaxSizeLane];
  int current_measure = -1;
  double current_beat = -1.0;

  c.GetNoteData().SortByBeat();

  for (auto& note: c.GetNoteData())
  {
    if (!CheckNoteValidity(note, param)) continue;

    NotePos& npos = note.pos();
    if (npos.beat != current_beat)
    {
      current_beat = npos.beat;
      double new_measure = c.GetTempoData().GetMeasureFromBeat(current_beat);
      if ((int)new_measure != current_measure)
      {
        current_measure = static_cast<int>(new_measure);
        GenerateRandomColumn(new_col, param);
      }
    }

    note.track.lane.note.lane = new_col[note.track.lane.note.lane];
  }
}

void RRandom(Chart &c, const EffectorParam& param, bool shift_by_timing)
{
  int lane_to_idx[kMaxSizeLane];
  int idx_to_lane[kMaxSizeLane];
  const int delta_lane = rand();
  SoundNote* longnote_lane[5][256];    // [ player index ][ lane ]
  ASSERT(param.lanesize < kMaxSizeLane);

  int shufflelanecnt = 0;
  for (int i=0; i<param.lanesize; i++)
  {
    if (param.lockedlane[i] == LaneTypes::NOTE)
    {
      lane_to_idx[i] = shufflelanecnt;
      idx_to_lane[shufflelanecnt] = i;
      shufflelanecnt++;
    }
  }

  ASSERT(shufflelanecnt == param.lanesize);

  c.GetNoteData().SortByBeat();

  constexpr double time_rotation_delta = 0.072;

  for (auto& note: c.GetNoteData())
  {
    if (!CheckNoteValidity(note, param)) continue;
    if (param.lockedlane[note.track.lane.note.lane]) continue;
    /**
     * Shift by timing option make note shifting by note's timing
     * which is used for SRANDOM option.
     */
    const size_t shift_amount = floorl(shift_by_timing ? time_rotation_delta : delta_lane);
    size_t new_idx = lane_to_idx[note.track.lane.note.lane] + shift_amount;
    while (longnote_lane[0][new_idx]) new_idx = (new_idx + 1) % param.lanesize;
    note.track.lane.note.lane = idx_to_lane[new_idx];
  }
}

void Mirror(Chart &c, const EffectorParam& param)
{
  int new_col[kMaxSizeLane];

  for (int i=0; i<kMaxSizeLane; i++) new_col[i] = i;

  // half of the lane is switched.
  int s=0, e=param.lanesize-1;
  while (s < e)
  {
    while (param.lockedlane[s] != NOTE && s < e) s++;
    while (param.lockedlane[e] != NOTE && s < e) e--;
    std::swap(new_col[s], new_col[e]);
    s++; e--;
  }

  c.GetNoteData().SortByBeat();

  for (auto& note: c.GetNoteData())
  {
    if (!CheckNoteValidity(note, param)) continue;
    note.track.lane.note.lane = new_col[note.track.lane.note.lane];
  }
}

void AllSC(Chart &c, const EffectorParam& param)
{
  std::vector<SoundNote*> row_notes;
  double current_beat = -1.0;
  int sc_idx = -1;
  for (int i=0; i<param.lanesize; i++)
  {
    if (param.lockedlane[i] == SC)
    {
      sc_idx = i;
      break;
    }
  }
  //ASSERT(sc_idx != -1);
  if (sc_idx == -1) return;

  c.GetNoteData().SortByBeat();

  for (auto& note: c.GetNoteData())
  {
    if (!CheckNoteValidity(note, param)) continue;
    
    NotePos& npos = note.pos();
    if (npos.beat != current_beat)
    {
      current_beat = npos.beat;
      if (row_notes.size())
        row_notes[rand() % row_notes.size()]->track.lane.note.lane = sc_idx;
      row_notes.clear();
    }

    row_notes.push_back(&note);
  }

  if (row_notes.size())
    row_notes[rand() % row_notes.size()]->track.lane.note.lane = sc_idx;
}

void Flip(Chart &c, const EffectorParam& param)
{
  int new_col[kMaxSizeLane];

  // half of the lane is switched.
  // XXX: ignores locked lane.
  for (int i=0; i<param.lanesize; i++)
  {
    new_col[i] = param.lanesize - i - 1;
  }

  c.GetNoteData().SortByBeat();

  for (auto& note: c.GetNoteData())
  {
    if (!CheckNoteValidity(note, param)) continue;
    note.track.lane.note.lane = new_col[note.track.lane.note.lane];
  }
}

} /* namespace effector */

} /* namespace rparser */
