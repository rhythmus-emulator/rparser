#include "ChartUtil.h"
#include "Song.h"
#include "common.h"

namespace rparser
{

class HTMLExporter
{
public:
  HTMLExporter();
  void AddNode(const char *name, const char *id_ = 0, const char *class_ = 0);
  void AddNodeWithNoChildren(const char *name, const char *id_ = 0, const char *class_ = 0, const char *text_ = 0);
  void AddNodeWithNoChildren(const char *name, const char *id_, const char *class_, const std::string &s);
  void AddRawText(const char *text);
  void PopNode();
  std::stringstream &ss();
  std::string str() const;
private:
  std::stringstream ss_;
  std::list<std::string> tag_stack_;
};

HTMLExporter::HTMLExporter() {}

void HTMLExporter::AddNode(const char *name, const char *id_, const char *class_)
{
  for (unsigned i = 0; i < tag_stack_.size(); ++i)
    ss_ << "\t";
  ss_ << "<" << name;
  if (id_)
    ss_ << " id='" << id_ << "'";
  if (class_)
    ss_ << " class='" << class_ << "'";
  ss_ << ">" << std::endl;
  tag_stack_.push_back(name);
}

void HTMLExporter::AddNodeWithNoChildren(const char *name, const char *id_, const char *class_, const char *text_)
{
  for (unsigned i = 0; i < tag_stack_.size(); ++i)
    ss_ << "\t";
  ss_ << "<" << name;
  if (id_)
    ss_ << " id='" << id_ << "'";
  if (class_)
    ss_ << " class='" << class_ << "'";
  if (text_)
  {
    ss_ << ">" << text_ << "<" << name;
  }
  ss_ << "/>" << std::endl;
}

void HTMLExporter::AddNodeWithNoChildren(const char *name, const char *id_, const char *class_, const std::string &s)
{
  AddNodeWithNoChildren(name, id_, class_, s.c_str());
}

void HTMLExporter::AddRawText(const char *text)
{
  ss() << text << std::endl;
}

std::stringstream &HTMLExporter::ss()
{
  for (unsigned i = 0; i < tag_stack_.size(); ++i)
    ss_ << "\t";
  return ss_;
}

void HTMLExporter::PopNode()
{
  ASSERT(tag_stack_.size() == 0);
  for (unsigned i = 0; i < tag_stack_.size(); ++i)
    ss_ << "\t";
  ss_ << "<" << tag_stack_.back() << "/>" << std::endl;
  tag_stack_.pop_back();
}

std::string HTMLExporter::str() const
{
  return ss_.str();
}

static inline void DrawLongnote(std::stringstream &ss, size_t &nd_idx, int track,
  const std::string &cls, const NoteElement &ne, double start, double end)
{
  ss << "<div id='nd" << nd_idx << "' class='" << cls << " longnote longnote_begin lane" << track <<
    "' style='top:" << (int)(start * 100) << "%'" << /* style end */
    " data-x=" << track << " data-y=" << (int)(start * 100) << " data-beat=" << ne.measure() << " data-time=" << ne.time() <<
    " data-id=" << nd_idx << " data-value=" << ne.get_value_i() << /* data end */
    "></div>";
  ss << "<div class='" << cls << " longnote longnote_body lane" << track <<
    "' style='top:" << (int)(start * 100) << "%; height:" << (int)((start - end) * 100) << "%'" << /* style end */
    " data-x=" << track << " data-beat=" << ne.measure() << " data-time=" << ne.time() <<
    " data-id=" << nd_idx << " data-value=" << ne.get_value_i() << /* data end */
    "></div>";
  ss << "<div class='" << cls << " longnote longnote_end lane" << track <<
    "' style='top:" << (int)(end * 100 + 1 /* round-up */) << "%'" << /* style end */
    " data-x=" << track << " data-beat=" << ne.measure() << " data-time=" << 0 <<
    " data-id=" << nd_idx << " data-value=" << ne.get_value_i() << /* data end */
    "></div>";
  nd_idx++;
}

static inline void DrawTapnote(std::stringstream &ss, size_t &nd_idx, int track,
  const std::string &cls, const NoteElement &ne, double p)
{
  ss << "<div id='nd" << nd_idx << "' class='" << cls << " tapnote lane" << track <<
    "' style='top:" << (int)(p * 100) << "%'" << /* style end */
    " data-x=" << track << " data-y=" << (int)(p * 100) << " data-beat=" << ne.measure() << " data-time=" << ne.time() <<
    " data-id=" << nd_idx << " data-value=" << ne.get_value_i() << /* data end */
    "></div>";
}

void ExportNoteToHTML(const Chart &c, HTMLExporter &e)
{
  auto &nd = c.GetNoteData();
  auto &tsd = c.GetTimingSegmentData();
  auto &cd = c.GetCommandData();
  auto &td = c.GetTimingData();
  auto &bars = tsd.GetBarObjects();

  e.AddNode("div", "content notedata flip", "notedata");

  // each measure has each div box
  // TODO: Display CommandData object
  uint32_t measure_idx = 0, bar_idx = 0;
  auto iter_note = const_cast<TrackData&>(nd).GetRowIterator();
  auto iter_tobj = const_cast<TrackData&>(td).GetRowIterator();
  size_t nd_idx = 0, td_idx = 0;
  bool is_longnote[256];
  uint32_t longnote_start_measure[256];
  double longnote_startpos[256];
  NoteElement *last_note[256];
  memset(is_longnote, 0, sizeof(is_longnote));
  memset(longnote_start_measure, 0, sizeof(longnote_start_measure));
  memset(longnote_startpos, 0, sizeof(longnote_startpos));
  memset(last_note, 0, sizeof(last_note));
  while (!iter_note.is_end() || !iter_tobj.is_end())
  {
    double barlength = tsd.GetBarLength(measure_idx);

    // measure start.
    e.ss() << "<div id='measure" << measure_idx << "' class='measurebox'" <<
      " data-measure=" << measure_idx << " data-length=" << barlength << " data-beat=" << measure_idx << ">";
    e.AddNode("div", 0, "inner");
    e.ss() << "<div class='measureno'>" << measure_idx << "</div>";

    // STEP 2-1-1. NoteData (General)
    std::string note_cls = "chartobject noteobject";
    while (!iter_note.is_end() && iter_note.get_measure() < measure_idx + 1)
    {
      for (size_t i = 0; i < nd.get_track_count(); ++i) if (iter_note.get(i))
      {
        /* XXX: don't use continue here; MUST go through full loop. */
        auto &n = iter_note[i];
        double ypos = n.measure() - (double)measure_idx;
        last_note[i] = &iter_note[i];

        /* if longnote_start, then remember starting position, then don't draw anything. */
        if (n.chain_status() == NoteChainStatus::Start)
        {
          longnote_startpos[i] = ypos;
          longnote_start_measure[i] = measure_idx;
          is_longnote[i] = true;
          continue;
        }

        bool is_longnote_end = is_longnote[i] && n.chain_status() == NoteChainStatus::End;
        if (is_longnote_end) /* check is longnote */
        {
          DrawLongnote(e.ss(), nd_idx, i, note_cls, n, longnote_startpos[i], ypos);
          is_longnote[i] = false;
        }
        else
        {
          DrawTapnote(e.ss(), nd_idx, i, note_cls, n, ypos);
        }
      }

      /* check for continuing longnote */
      for (size_t i = 0; i < nd.get_track_count(); ++i)
      {
        if (is_longnote[i])
          DrawLongnote(e.ss(), nd_idx, i, "chartobject noteobject", *last_note[i], longnote_startpos[i], 1.0);
        longnote_startpos[0] = 0;
      }

      ++iter_note;
    }

    // STEP 2-2. TempoData
    while (!iter_tobj.is_end() && iter_tobj.get_measure() < measure_idx + 1)
    {
      for (size_t i = 0; i < td.get_track_count(); ++i) if (iter_tobj.get(i))
      {
        note_cls = "chartobject tempoobject tempotype0";
        note_cls.back() += i;
        auto &n = iter_tobj[i];
        double ypos = n.measure() - (double)measure_idx;
        DrawTapnote(e.ss(), td_idx, i, note_cls, n, ypos);
        ++iter_tobj;
      }
    }

    // measure end.
    e.PopNode();
    e.ss() << "</div>";
    measure_idx++;
  }

  e.PopNode();
}

void ExportToHTML(const Chart &c, std::string& out)
{
  HTMLExporter e;

  auto &md = c.GetMetaData();
  auto &nd = c.GetNoteData();
  auto &cd = c.GetCommandData();
  auto &tsd = c.GetTimingSegmentData();
  auto &td = c.GetTimingData();

  // STEP 0. Container
  {
    std::stringstream ss;
    ss << "playtype - " << ChartTypeToString(c.GetChartType()) << " playlane-" << (int)c.GetPlayLaneCount() << "key";
    e.AddNode("div", "rhythmus-container", ss.str().c_str());
  }

  // STEP 1-1. Metadata
  e.AddNode("div", "metadata", "content metadata");
  {
    e.AddNodeWithNoChildren("span", 0, "title", "Metadata Info");
    // meta related with song
    e.AddNode("span", 0, "desc meta_title");
    e.AddNodeWithNoChildren("span", 0, "label", "Filename");
    e.AddNodeWithNoChildren("span", 0, "text", c.GetFilename());
    e.PopNode();
    e.AddNode("span", 0, "desc meta_filetype");
    e.AddNodeWithNoChildren("span", 0, "label", "Filetype");
    e.AddNodeWithNoChildren("span", 0, "text", ChartTypeToString(c.GetChartType()));
    e.PopNode();
    e.AddNode("span", 0, "desc meta_playmode");
    e.AddNodeWithNoChildren("span", 0, "label", "PlayMode");
    e.AddNodeWithNoChildren("span", 0, "text", std::to_string((int)c.GetPlayLaneCount()));
    e.PopNode();
    // meta related with metadata
    e.ss() << "<span class='desc meta_title'><span class='label'>Title</span><span class='text'>" << md.title <<
      "<span class='meta_subtitle'>" << md.subtitle << "</span>" << "</span></span>";
    e.ss() << "<span class='desc meta_artist'><span class='label'>Artist</span><span class='text'>" << md.artist <<
      "<span class='meta_subartist'>" << md.subartist << "</span>" << "</span></span>";
    e.ss() << "<span class='desc meta_level'><span class='label'>Level</span><span class='text'>" << md.level << "</span></span>";
    e.ss() << "<span class='desc meta_bpm'><span class='label'>BPM</span><span class='text'>" << md.bpm << "</span></span>";
    e.ss() << "<span class='desc meta_total'><span class='label'>Gauge Total</span><span class='text'>" << md.gauge_total << "</span></span>";
    e.ss() << "<span class='desc meta_diff'><span class='label'>Difficulty</span><span class='text'>" << md.difficulty << "</span></span>";
    // meta related with notedata
    e.ss() << "<span class='desc meta_notecount'><span class='label'>Note Count</span><span class='text'>" << c.GetScoreableNoteCount() << "</span></span>";
    // meta related with eventdata
    e.ss() << "<span class='desc meta_eventcount'><span class='label'>Event Count</span><span class='text'>" << cd.size() << "</span></span>";
    // meta related with tempodata
    e.ss() << "<span class='desc meta_maxbpm'><span class='label'>Max BPM</span><span class='text'>" << tsd.GetMaxBpm() << "</span></span>";
    e.ss() << "<span class='desc meta_minbpm'><span class='label'>Min BPM</span><span class='text'>" << tsd.GetMinBpm() << "</span></span>";
    e.ss() << "<span class='desc meta_isbpmchange'><span class='label'>BPM Change?</span><span class='text'>" << (tsd.HasBpmChange() ? "Yes" : "No") << "</span></span>";
    e.ss() << "<span class='desc meta_hasstop'><span class='label'>BPM Change?</span><span class='text'>" << (tsd.HasStop() ? "Yes" : "No") << "</span></span>";
    e.ss() << "<span class='desc meta_haswarp'><span class='label'>STOP?</span><span class='text'>" << (tsd.HasWarp() ? "Yes" : "No") << "</span></span>";
    e.ss() << "<span class='desc meta_haswarp'><span class='label'>WARP?</span><span class='text'>" << (tsd.HasWarp() ? "Yes" : "No") << "</span></span>";
    uint32_t lasttime = static_cast<uint32_t>(c.GetSongLastObjectTime());
    char lasttime_str[3][3];
    sprintf(lasttime_str[0], "%02d", lasttime / 3600000);
    sprintf(lasttime_str[1], "%02d", lasttime / 60000 % 60);
    sprintf(lasttime_str[2], "%02d", lasttime / 1000 % 60);
    e.ss() << "<span class='desc meta_songlength'><span class='label'>Song Length</span><span class='text'>" <<
      lasttime_str[0] << ":" << lasttime_str[1] << ":" << lasttime_str[2] << "</span></span>";
    // meta - BMS conditional statements
    if (!md.script.empty())
    {
      e.ss() << "<span class='desc meta_script'><span class='label'>Script</span><span class='text'>...</span><span class='text hide'>" <<
        md.script << "</span></span>";
    }
  }
  e.PopNode();

  // STEP 1-2. Metadata (Resource)
  e.AddNode("div", "resourcedata", "content resourcedata");
  {
    e.AddNodeWithNoChildren("span", 0, "title", "Resource Info");

    e.AddNode("ul", "soundresource");
    for (auto ii : md.GetSoundChannel()->fn)
    {
      e.ss() << "<li data-channel='" << ii.first << "' data-value='" << ii.second <<
        "' >Channel " << ii.first << ", " << ii.second << "</li>";
    }
    e.PopNode();

    e.AddNode("ul", "bgaresource");
    for (auto ii : md.GetBGAChannel()->bga)
    {
      e.ss() << "<li data-channel='" << ii.first << "' data-value='" << ii.second.fn <<
        "' >Channel " << ii.first << ", " << ii.second.fn << "</li>";
    }
    e.PopNode();

    e.AddNode("ul", "bpmresource");
    for (auto ii : md.GetBPMChannel()->bpm)
    {
      e.ss() << "<li data-channel='" << ii.first << "' data-value='" << ii.second <<
        "' >Channel " << ii.first << ", " << ii.second << "</li>";
    }
    e.PopNode();

    e.AddNode("ul", "stopresource");
    for (auto ii : md.GetSTOPChannel()->stop)
    {
      e.ss() << "<li data-channel='" << ii.first << "' data-value='" << ii.second <<
        "' >Channel " << ii.first << ", " << ii.second << "</li>";
    }
    e.PopNode();
  }
  e.PopNode(); /* div.resourcedata */

  // STEP 2. Render objects (base object)
  ExportNoteToHTML(c, e);

  // -- end --
  e.PopNode();

  out = e.str();
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
  size_t track_count = c.GetNoteData().get_track_count();
  TrackData track_new;

  track_new.set_track_count(track_count);
  auto iter = c.GetNoteData().GetRowIterator();
  int measure_idx = -1;
  while (!iter.is_end())
  {
    // TODO: Longnote processing?
    for (size_t i = 0; i < track_count; ++i) if (iter.get(i))
    {
      auto &note = iter[i];
      int curr_measure_idx = static_cast<int>(note.measure());
      if (measure_idx != curr_measure_idx)
      {
        measure_idx = curr_measure_idx;
        if (!c.GetNoteData().IsHoldNoteAt(note.measure()))
          GenerateRandomColumn(new_col, param);
      }
      track_new[new_col[i]].AddNoteElement(note);
    }
    ++iter;
  }

  c.GetNoteData().swap(track_new);
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
  TrackData track_new;
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
  const size_t track_count = c.GetNoteData().get_track_count();
  bool change_mapping = true;
  size_t shift_idx = 0;
  track_new.set_track_count(track_count);
  while (!rowiter.is_end())
  {
    /* Only shift if no longnote in current row. */
    change_mapping = !rowiter.is_longnote();
    for (size_t i = 0; i < track_count; ++i) if (rowiter.get(i))
    {
      /**
        * Shift by timing option make note shifting by note's timing
        * which is used for SRANDOM option.
        */
      if (param.lockedlane[i])
        continue;
      NoteElement &n = rowiter[i];
      size_t new_idx;

      if (change_mapping)
      {
        shift_idx = (size_t)floorl(mapping_by_measure
          ? n.time() / time_rotation_delta
          : delta_lane + n.measure());
      }

      new_idx = lane_to_idx[i] + shift_idx;
      new_idx = (new_idx + 1) % param.lanesize;
      track_new[idx_to_lane[new_idx]].AddNoteElement(n);
    }
    ++rowiter;
  }

  c.GetNoteData().swap(track_new);
}

void Mirror(Chart &c, const EffectorParam& param)
{
  TrackData track_new;
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

  track_new.set_track_count(c.GetNoteData().get_track_count());
  for (auto i = 0; i < track_new.get_track_count(); ++i)
    track_new[new_col[i]].swap(c.GetNoteData()[i]);
  c.GetNoteData().swap(track_new);
}

void AllSC(Chart &c, const EffectorParam& param)
{
  double current_beat = -1.0;
  int sc_idx = -1;
  size_t scan_col_start = 0;
  TrackData track_new;

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

  track_new.set_track_count(c.GetNoteData().get_track_count());
  auto rowiter = c.GetNoteData().GetRowIterator();
  while (!rowiter.is_end())
  {
    /* scratch column should be empty. */
    if (rowiter.get(sc_idx) != nullptr)
      continue;
    for (size_t i = 0; i < param.lanesize; ++i)
    {
      bool is_sc_filled = false;
      int colidx = (i + scan_col_start) % param.lanesize;

      NoteElement *n = rowiter.get(colidx);
      if (!n) continue;
      if (!is_sc_filled && n->chain_status() == NoteChainStatus::Tap) /* if not longnote */
      {
        // then copy note to scratch lane.
        track_new[sc_idx].AddNoteElement(*n);
      }
      else
      {
        // if not, then copy as it is
        track_new[colidx].AddNoteElement(*n);
      }
    }
    scan_col_start++;
    ++rowiter;
  }
  c.GetNoteData().swap(track_new);
}

void Flip(Chart &c, const EffectorParam& param)
{
  TrackData track_new;
  int new_col[kMaxSizeLane];

  // half of the lane is switched.
  // XXX: ignores locked lane.
  for (int i=0; i<param.lanesize; i++)
  {
    new_col[i] = param.lanesize - i - 1;
  }

  track_new.set_track_count(c.GetNoteData().get_track_count());
  for (auto i = 0; i < track_new.get_track_count(); ++i)
    track_new[new_col[i]].swap(c.GetNoteData()[i]);
  c.GetNoteData().swap(track_new);
}

} /* namespace effector */

} /* namespace rparser */
