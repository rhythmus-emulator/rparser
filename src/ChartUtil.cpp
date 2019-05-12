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
  if (note.GetNotetype() != NoteTypes::kNote)
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

    NotePos& npos = note.GetNotePos();
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
    const size_t shift_amount = shift_by_timing ? time_rotation_delta : delta_lane;
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
    
    NotePos& npos = note.GetNotePos();
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
