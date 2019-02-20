#include "ChartUtil.h"

namespace rparser
{

void NoteSelection::SelectNote(Note* n)
{
	auto it = std::lower_bound(m_vNotes.begin(), m_vNotes.end(), n, nptr_less_row());
	m_vNotes.insert(it, n);
}
void NoteSelection::UnSelectNote(const Note* n)
{
	for (int i = 0; i < m_vNotes.size(); i++)
	{
		if (*m_vNotes[i] == *n)
		{
			m_vNotes.erase(i + m_vNotes.begin());
			break;
		}
	}
}
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

void NoteData::TrackMapping(int *trackmap, int s, int e)
{
  // only map for TAPNOTE
  for (auto &n : m_Track)
  {
    if (n.nType != NoteType::NOTE_TAP) continue;
    if (n.x < s || n.x >= e) continue;
    n.x = trackmap[n.x - s] + s;
  }
}

void NoteData::TrackSRandom(int side, int key, bool bHrandom)
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

void FixImpossibleNote()
{
  
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

}
