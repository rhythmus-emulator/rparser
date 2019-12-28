#include "ChartUtil.h"
#include "Song.h"
#include <list>

namespace rparser
{

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

void ExportNoteToHTML(const Chart &c, HTMLExporter &e)
{
  auto &nd = c.GetNoteData();
  auto &td = c.GetTimingSegmentData();
  auto &tnd = td.GetTimingData();
  auto &bars = td.GetBarObjects();
  std::list<const Note*> longnotes;

  e.line() << "<div class='content notedata flip' id='notedata'>";
  e.PushIndent();

  // each measure has each div box
  uint32_t measure_idx = 0, bar_idx = 0;
  auto iter_note = nd.GetAllTrackIterator();
  auto iter_tobj = tnd.GetAllTrackIterator();
  size_t nd_idx = 0, td_idx = 0;
  while (!iter_note.is_end() || !iter_tobj.is_end())
  {
    double barlength = td.GetBarLength(measure_idx);

    // measure start.
    e.line() << "<div id='measure" << measure_idx << "' class='measurebox'" <<
      " data-measure=" << measure_idx << " data-length=" << barlength << " data-beat=" << measure_idx << "><div class='inner'>";
    e.PushIndent();
    e.line() << "<div class='measureno'>" << measure_idx << "</div>";

    // STEP 2-1-0. NoteData (Continuing Longnote)
    auto lii = longnotes.begin();
    while (lii != longnotes.end())
    {
      const auto &n = **lii;
      double endrow = n.endpos().measure;
      bool is_longnote_end_here = true;
      if (endrow > measure_idx)
      {
        endrow = measure_idx;
        is_longnote_end_here = false;
      }
      double endpos = (measure_idx - endrow) * 100 + 1 /* round-up */;
      e.line() << "<div class='chartobject noteobject longnote longnote_body lane" << (int)n.get_track() <<
        "' style='top:0%; height:" << (int)endpos << "%'" << /* style end */
        " data-x=" << (int)n.get_track() << " data-beat=" << n.measure << " data-time=" << n.time_msec <<
        " data-id=" << nd_idx - 1 << " data-value=" << n.channel() << /* data end */
        "></div>";
      if (is_longnote_end_here)
      {
        e.line() << "<div class='chartobject noteobject longnote longnote_end lane" << (int)n.get_track() <<
          "' style='top:" << (int)endpos << "%'" << /* style end */
          " data-x=" << (int)n.get_track() << " data-beat=" << n.measure << " data-time=" << n.time_msec <<
          " data-id=" << nd_idx - 1 << " data-value=" << n.channel() << /* data end */
          "></div>";
      }

      auto preii = lii++;
      if (is_longnote_end_here)
        longnotes.erase(preii);
    }

    // STEP 2-1-1. NoteData (General)
    while (!iter_note.is_end() && (*iter_note).measure < measure_idx + 1)
    {
      /* XXX: don't use continue here; MUST go through full loop. */
      auto &n = static_cast<Note&>(*iter_note);
      double ypos = (n.measure - (double)measure_idx) * 100;
      bool is_longnote = n.chainsize() > 1;
      std::string cls = "chartobject noteobject";
      if (is_longnote) /* check is longnote */
        cls += " longnote longnote_begin";
      else
        cls += " tapnote";
      e.line() << "<div id='nd" << nd_idx << "' class='" << cls << " lane" << n.get_track() <<
        "' style='top:" << (int)ypos << "%'" << /* style end */
        " data-x=" << n.get_track() << " data-y=" << (int)ypos << " data-beat=" << n.measure << " data-time=" << n.time_msec <<
        " data-id=" << nd_idx << " data-value=" << n.channel() << /* data end */
        "></div>";
      if (is_longnote)
      {
        // draw longnote body & end
        double endrow = n.endpos().measure;
        bool is_longnote_end_here = true;
        if (endrow > measure_idx)
        {
          endrow = measure_idx;
          is_longnote_end_here = false;
          longnotes.push_back(&n);
        }
        double endpos = (endrow - (double)measure_idx) * 100 + 1 /* round-up */;
        e.line() << "<div class='chartobject noteobject longnote longnote_body lane" << n.get_track() <<
          "' style='top:" << (int)ypos << "%; height:" << (int)endpos << "%'" << /* style end */
          " data-x=" << n.get_track() << " data-beat=" << n.measure << " data-time=" << n.time_msec <<
          " data-id=" << nd_idx << " data-value=" << n.channel() << /* data end */
          "></div>";
        if (is_longnote_end_here)
        {
          e.line() << "<div class='chartobject noteobject longnote longnote_end lane" << n.get_track() <<
            "' style='top:" << (int)(ypos + endpos) << "%'" << /* style end */
            " data-x=" << n.get_track() << " data-beat=" << n.measure << " data-time=" << n.time_msec <<
            " data-id=" << nd_idx << " data-value=" << n.channel() << /* data end */
            "></div>";
        }
      }
      ++nd_idx;
      ++iter_note;
    }

    // STEP 2-2. TempoData
    while (!iter_tobj.is_end() && (*iter_tobj).measure < measure_idx + 1)
    {
      auto &n = static_cast<TimingObject&>(*iter_tobj);
      double ypos = (n.measure - (double)measure_idx) * 100;
      e.line() << "<div id='td" << td_idx << "' class='chartobject tempoobject tempotype" << n.get_track() <<
        "' style='top:" << (int)ypos << "%'" << /* style end */
        " data-y=" << (int)ypos << " data-beat=" << n.measure << " data-time=" << n.time_msec << /* data end */
        "></div>";
      ++td_idx;
      ++iter_tobj;
    }

    // measure end.
    e.PopIndent();
    e.line() << "</div></div>";
    measure_idx++;
  }

  e.PopIndent();
  e.line() << "</div>";
}

void ExportToHTML(const Chart &c, std::string& out)
{
  HTMLExporter e;

  auto &md = c.GetMetaData();
  auto &nd = c.GetNoteData();
  auto &ed = c.GetEffectData();
  auto &td = c.GetTimingSegmentData();
  auto &tnd = td.GetTimingData();

  // STEP 0. Container
  e.line() << "<div id='rhythmus-container' class='playtype-" << ChartTypeToString(c.GetChartType()) <<
    " playlane-" << (int)c.GetPlayLaneCount() << "key'>";
  e.PushIndent();

  // STEP 1-1. Metadata
  {
    e.line() << "<div id='metadata' class='content metadata'>";
    e.PushIndent();
    e.line() << "<span class='title'>Metadata Info</span>";
    // meta related with song
    e.line() << "<span class='desc meta_title'><span class='label'>Filename</span><span class='text'>" << c.GetFilename() << "</span></span>";
    e.line() << "<span class='desc meta_filetype'><span class='label'>Filetype</span><span class='text'>" << ChartTypeToString(c.GetChartType()) << "</span></span>";
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
    // meta - BMS conditional statements
    if (!md.script.empty())
    {
      e.line() << "<span class='desc meta_script'><span class='label'>Script</span><span class='text'>...</span><span class='text hide'>" <<
        md.script << "</span></span>";
    }
    e.PopIndent();
    e.line() << "</div>";
  }

  // STEP 1-2. Metadata (Resource)
  {
    e.line() << "<div id='resourcedata' class='content resourcedata'>";
    e.PushIndent();

    e.line() << "<span class='title'>Resource Info</span>";

    e.line() << "<ul id='soundresource'>";
    e.PushIndent();
    for (auto ii : md.GetSoundChannel()->fn)
    {
      e.line() << "<li data-channel='" << ii.first << "' data-value='" << ii.second <<
        "' >Channel " << ii.first << ", " << ii.second << "</li>";
    }
    e.PopIndent();
    e.line() << "</ul>";

    e.line() << "<ul id='bgaresource'>";
    e.PushIndent();
    for (auto ii : md.GetBGAChannel()->bga)
    {
      e.line() << "<li data-channel='" << ii.first << "' data-value='" << ii.second.fn <<
        "' >Channel " << ii.first << ", " << ii.second.fn << "</li>";
    }
    e.PopIndent();
    e.line() << "</ul>";

    e.line() << "<ul id='bpmresource'>";
    e.PushIndent();
    for (auto ii : md.GetBPMChannel()->bpm)
    {
      e.line() << "<li data-channel='" << ii.first << "' data-value='" << ii.second <<
        "' >Channel " << ii.first << ", " << ii.second << "</li>";
    }
    e.PopIndent();
    e.line() << "</ul>";

    e.line() << "<ul id='stopresource'>";
    e.PushIndent();
    for (auto ii : md.GetSTOPChannel()->stop)
    {
      e.line() << "<li data-channel='" << ii.first << "' data-value='" << ii.second <<
        "' >Channel " << ii.first << ", " << ii.second << "</li>";
    }
    e.PopIndent();
    e.line() << "</ul>";

    e.PopIndent();
    e.line() << "</div>";
  }

  // STEP 2. Render objects (base object)
  ExportNoteToHTML(c, e);

  // -- end --
  e.PopIndent();
  e.line() << "</div>";

  out = e.line().str();
}

void ClipRange(Chart &c, double beat_start, double beat_end)
{
  // TODO
  // XXX: Add "CopyRange" ?
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
  rutil::Random random;
  random.SetSeed(param.seed);
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
      new_col[i] = random.Next() % lanes_to_randomize + lockedcnt;
      lockedcnt++;
      lanes_to_randomize--;
    }
  }
  ASSERT(lanes_to_randomize == 0);
}

inline bool CheckNoteValidity(Note& note, const EffectorParam& param)
{
  int current_col = note.get_track();
  int current_player = note.get_player();
  if (current_player != param.player)
    return false;
  ASSERT(note.get_track() < kMaxSizeLane);
  return true;
}

void Random(Chart &c, const EffectorParam& param)
{
  int new_col[kMaxSizeLane];

  GenerateRandomColumn(new_col, param);

  c.GetNoteData().RemapTracks((size_t*)new_col);
}

void SRandom(Chart &c, const EffectorParam& param)
{
  // SRANDOM algorithm from
  // https://gall.dcinside.com/mgallery/board/view/?id=popn&no=3291&page=1484
  // XXX: may need to use Random context consequentially?
  Random(c, param);
  RRandom(c, param, true);
  Random(c, param);
}

/**
 * @brief Shuffle tracks random for each measure.
 * @warn Won't change track index if longnote is spaned between measures.
 *       to prevent generating wrong note chart.
 *       (e.g. tapnote between longnote)
 */
void HRandom(Chart &c, const EffectorParam& param)
{
  int new_col[kMaxSizeLane];
  int current_measure = -1;
  double current_beat = -1.0;

  auto iter = c.GetNoteData().GetAllTrackIterator();
  int measure_idx = -1;
  while (!iter.is_end())
  {
    auto &note = static_cast<Note&>(*iter);
    int curr_measure_idx = static_cast<int>(note.measure);
    if (measure_idx != curr_measure_idx)
    {
      measure_idx = curr_measure_idx;
      if (!c.GetNoteData().IsHoldNoteAt(note.measure))
        GenerateRandomColumn(new_col, param);
    }
    note.set_track(new_col[note.get_track()]);
  }

  c.GetNoteData().UpdateTracks();
}

/**
 * @brief Shift tracks sequentially by measure or time.
 * @warn Won't change track index if longnote is spaned between measures/times.
 *       to prevent generating wrong note chart.
 *       (e.g. tapnote between longnote)
 * @param mapping_by_measure mapping by measure(true) or by notetime(false).
 */
void RRandom(Chart &c, const EffectorParam& param, bool mapping_by_measure)
{
  rutil::Random random;
  int lane_to_idx[kMaxSizeLane];
  int idx_to_lane[kMaxSizeLane];
  random.SetSeed(param.seed);
  const int delta_lane = random.Next();
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

  constexpr double time_rotation_delta = 0.072;

  auto rowiter = c.GetNoteData().GetRowIterator();
  bool change_mapping = true;
  size_t shift_idx = 0;
  while (!rowiter.is_end())
  {
    /* Only shift if no longnote in current row. */
    change_mapping = !c.GetNoteData().IsHoldNoteAt(rowiter.get_measure());

    for (int i = 0; i < param.lanesize; ++i)
    {
      /**
       * Shift by timing option make note shifting by note's timing
       * which is used for SRANDOM option.
       */
      if (param.lockedlane[i] || rowiter.col(i) == nullptr)
        continue;
      Note &n = static_cast<Note&>(*rowiter.col(i));
      size_t new_idx;

      if (change_mapping)
      {
        shift_idx = (size_t)floorl(mapping_by_measure
            ? n.time_msec / time_rotation_delta
            : delta_lane + n.measure);
      }

      new_idx = lane_to_idx[n.get_track()] + shift_idx;
      new_idx = (new_idx + 1) % param.lanesize;
      n.set_track(idx_to_lane[new_idx]);
    }
  }

  c.GetNoteData().UpdateTracks();
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

  auto iter = c.GetNoteData().GetAllTrackIterator();
  while (!iter.is_end())
  {
    auto &note = static_cast<Note&>(*iter);
    if (!CheckNoteValidity(note, param)) continue;
    note.set_track(new_col[note.get_track()]);
  }

  c.GetNoteData().UpdateTracks();
}

void AllSC(Chart &c, const EffectorParam& param)
{
  double current_beat = -1.0;
  int sc_idx = -1;
  size_t scan_col_start = 0;

  // set scratch index.
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

  auto rowiter = c.GetNoteData().GetRowIterator();
  while (!rowiter.is_end())
  {
    /* scratch column should be empty. */
    if (rowiter.col(sc_idx) != nullptr)
      continue;
    for (int i = 0; i < param.lanesize; ++i)
    {
      int colidx = (i + scan_col_start) % param.lanesize;
      Note *n = static_cast<Note*>(rowiter.col(colidx));
      if (n != nullptr && n->chainsize() == 1) /* if not longnote */
      {
        n->set_track(sc_idx);
      }
    }
    scan_col_start++;
  }

  c.GetNoteData().UpdateTracks();
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

  auto iter = c.GetNoteData().GetAllTrackIterator();
  while (!iter.is_end())
  {
    auto &note = static_cast<Note&>(*iter);
    if (!CheckNoteValidity(note, param)) continue;
    note.set_track(new_col[note.get_track()]);
  }

  c.GetNoteData().UpdateTracks();
}

} /* namespace effector */

} /* namespace rparser */
