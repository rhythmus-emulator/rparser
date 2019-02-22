#include "ChartUtil.h"
#include "Song.h"

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

namespace effector
{

// TODO: use NoteSelection.
void Random(Chart &c, int player)
{
  int col_random[14] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13
  };
  std::random_shuffle(col_random, col_random + key);
  int current_col;
  int current_player;
  for (auto& note: c.GetNoteData())
  {
    current_col = note.track.lane.note.col;
    current_player = note.track.lane.note.player;
    if (note.track.type != NoteTypes::kNote)
      continue;
    if (note.track.subtype != NoteTypes::kNote)
      continue;
    if (current_player != player)
      continue;
    ASSERT(note.track.lane.note.col < 14);
    note.track.lane.note.col = col_random[note.track.lane.note.col];
  }
}

void SRandom(int side, int key, bool bHrandom)
{
  ASSERT(key < 10 && key > 0 && side >= 0 && side < 2);
  int vShuffle[10] = {
    0,1,2,3,4,5,6,7,8,9
  };
  float curr_LN_length[10];   // LN length in beat
  for (int i = 0; i < 10; i++) curr_LN_length[i] = 0;

  float curr_beat = -100;		// current lane's beat
  int curr_side = 0;			// current note's lane side
  int curr_shuffle_idx = 0;	// shuffling index
  int curr_note_cnt = 0;		// note count of current beat
  for (auto &n : m_Track) {
    if (n.nType != NoteType::NOTE_TAP) continue;
    curr_side = n.x / 10;
    if (curr_side != side) continue;
    if (curr_beat != n.fBeat)
    {
      // new beat? then shuffle value.
      // (no HRANDOM)
      if (!bHrandom)
      {
        std::random_shuffle(vShuffle, vShuffle + key);
        curr_shuffle_idx = 0;
      }
      float delta_beat = n.fBeat - curr_beat;
      curr_beat = n.fBeat;
      // check longnote ends
      for (int i = 0; i < key; i++)
      {
        curr_LN_length[i] -= delta_beat;
        if (curr_LN_length[i] < 0) curr_LN_length[i] = 0;
      }
      curr_note_cnt = 0;
    }
    // prevent duplication with LN
    while (curr_LN_length[vShuffle[curr_shuffle_idx]] > 0
      && curr_shuffle_idx < key)
    {
      curr_shuffle_idx++;
      curr_note_cnt++;
    }
    if (curr_shuffle_idx >= key)
    {
      // this part will work in case of H-RANDOM
      // prevent note jackhammering by minimizing note shuffling
      // COMMENT:
      // If current lane contains 2 Notes (in 9 keys),
      // Then shuffle 7 note backward and 2 note for forward
      // to minimize note hammering.
      std::random_shuffle(vShuffle, vShuffle + curr_note_cnt);
      std::random_shuffle(vShuffle + curr_note_cnt, vShuffle + key - curr_note_cnt);
      curr_shuffle_idx = 0;
    }
    n.x = vShuffle[curr_shuffle_idx] + curr_side * 10;
    curr_shuffle_idx++;
    curr_note_cnt++;
    // if current note is LN, then remember
    if (n.subType == NoteTapType::TAPNOTE_CHARGE)
    {
      curr_LN_length[vShuffle[curr_shuffle_idx]] = n.fBeatLength;
    }
  }
}

void LaneRandom(int side, int key)
{
  ASSERT(side >= 0 && key >= 0 && key < 10);
  int vShuffle[10] = { 0,1,2,3,4,5,6,7,8,9 };
  std::random_shuffle(vShuffle, vShuffle + key);
  TrackMapping(vShuffle, side * 10, side * 10 + key);
}

void FixInvalidNote(Chart &c, SONGTYPE songtype, bool delete_invalid_note)
{
  // TODO: currently for note type.
  //       in case of pos type ...?
  unsigned int player_count = c.GetMetaData().player_count;
  unsigned int track_column_count = GetSongMetaData(songtype).track_col_count;
  double current_beat = -1;
  Note* note_using_lane[5][256];
  int current_col;
  int current_player;

  ASSERT(player_count < 5);
  ASSERT(track_column_count < 128);

  // must sorted first
  c.GetNoteData().SortByBeat();

  memset(note_using_lane, 0, sizeof(note_using_lane));

  for (auto& note: c.GetNoteData())
  {
    current_col = note.track.lane.note.col;
    current_player = note.track.lane.note.player;
    if (note.track.type != NoteTypes::kNote)
      continue;
    if (note.track.subtype != NoteTypes::kNote)
      continue;
    // if beat change, invalidate all marked note status
    if (note.pos.beat != current_beat)
    {
      current_beat = note.pos.beat;
      for (int pi=0 ; pi<player_count ; pi++)
      {
        for (int i=0 ; i<track_column_count ; i++)
        {
          Note** ncl = &note_using_lane[pi][i];
          if (!(*ncl)->next)
          {
            // not longnote (or end of longnote), instantly clear.
            (*ncl) = 0;
          }
          else
          {
            // in case of longnote, renew longnote object.
            // TODO: in case of pos_end attribute note?
            if ((*ncl)->pos.beat < current_beat)
              (*ncl) = (*ncl)->next;
          }
        }
      }
    }
    // attempt to put note in empty lane.
    // if lane used, then attempt to next (right) lane.
    // if proper lane not found, then note won't edited. (stay in incorrect one)
    for (int i=0; i<track_column_count; i++)
    {
      int new_col = (current_col + i) % track_column_count;
      if (!note_using_lane[current_player][new_col])
      {
        note.track.lane.note.col = new_col;
        // in case of longnote, fix them all.
        Note *ln_note = note.next;
        while (ln_note)
        {
          ln_note->track.lane.note.col = new_col;
        } while (ln_note = ln_note->next)
        note_using_lane[current_player][new_col] = &note;
        break;
      }
    }
  }
}

void NoteData::TrackRRandom(int side, int key)
{
  ASSERT(side >= 0 && key >= 0 && key < 10);
  int vShuffle[10] = { 0,1,2,3,4,5,6,7,8,9 };
  int iMoveVal = rand()*key;
  for (int i = 0; i < key; i++)
  {
    vShuffle[i] -= iMoveVal;
    if (vShuffle[i] < 0) vShuffle[i] += key;
  }
  TrackMapping(vShuffle, side * 10, side * 10 + key);
}

void NoteData::TrackMirror(int side, int key)
{
  ASSERT(side >= 0 && key >= 0 && key < 10);
  int vTrackmap[10];
  for (int i = 0; i < key; i++)
  {
    vTrackmap[i] = key - i - 1;
  }
  TrackMapping(vTrackmap, side * 10, side * 10 + key);
}

void NoteData::TrackAllSC(int side)
{
  float curr_beat = -100;		// current beat (to detect beat changing)
  int curr_sc = 0;			// current row has SC?
  std::vector<Note*> vNotes_sc_candidate;
  for (auto &n : m_Track)
  {
    if (n.fBeat != curr_beat)
    {
      // select one note and make it SC
      if (curr_sc == 0)
      {
        Note* n_sc_candidate = vNotes_sc_candidate[vNotes_sc_candidate.size() * rand()];
        n_sc_candidate->x = 0;
      }
      // prepare for next
      curr_sc = 0;
      curr_beat = n.fBeat;
      vNotes_sc_candidate.clear();
    }
    else if (curr_sc) {
      continue;
    }
    // SC channel : 0
    if (n.x == 0)
    {
      curr_sc = 1;
      continue;
    }
    vNotes_sc_candidate.push_back(&n);
  }
}

} /* namespace effector */

} /* namespace rparser */
