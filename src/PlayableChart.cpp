#include "PlayableChart.h"

namespace rparser
{

namespace playablechart
{

void FillNoteTimingData(std::vector<Note>& vNotes, const TimingData& td)
{
  float curr_beat = -1;
  double curr_row_second = -1.0;
  Note* curr_note;
  for (int i = 0; i < vNotes.size(); ++i)
  {
    curr_note = &vNotes[i];
    // time calculation for current row
    if (curr_note->fBeat != curr_beat)
    {
      curr_beat = curr_note->fBeat;
      curr_row_second = td.LookupMSecFromBeat(curr_beat);
    }
    if (curr_note->nType != NoteType::NOTE_EMPTY)
    {
      curr_note->fTime = curr_row_second;
      // in case of hold note, calculate end time too
      if (curr_note->subType == NoteTapType::TAPNOTE_CHARGE)
      {
        curr_note->fTimeLength = td.LookupMSecFromBeat(curr_beat + curr_note->fBeatLength);
      }
    }
  }
}

}

}
