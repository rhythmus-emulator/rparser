#include "NoteData.h"
#include <algorithm>

void NoteData::SearchAllNotes(std::vector<trackiter>& notelist);
void NoteData::SearchAllNotes(std::vector<trackiter>& notelist, int iStartRow, int iEndRow, bool bInclusive);

// @description fill all note's timing data from beat data.
void NoteData::FillTimingData(const TimingData& td)
{
    td.PrepareLookup();
    
	int curr_row= -1;
	NoteData::all_tracks_iterator curr_note=
		GetTapNoteRangeAllTracks(0, MAX_NOTE_ROW);
	vector<TapNote*> notes_on_curr_row;
	TapNoteSubType highest_subtype_on_row= TapNoteSubType_Invalid;
	double curr_row_second= -1.0;
	vector<int> column_ids(GetNumTracks(), 0);
	int curr_note_id= 0;
	int curr_row_id= -1; // Start at -1 so that the first row update sets to 0.
	while(!curr_note.IsAtEnd())
	{
		if(curr_note.Row() != curr_row)
		{
			for(auto&& note : notes_on_curr_row)
			{
				note->highest_subtype_on_row= highest_subtype_on_row;
			}
			notes_on_curr_row.clear();
			highest_subtype_on_row= TapNoteSubType_Invalid;
			curr_row= curr_note.Row();
			curr_row_second= timing_data->GetElapsedTimeFromBeat(NoteRowToBeat(curr_row));
			++curr_row_id;
		}
		if(curr_note->type != TapNoteType_Empty)
		{
			curr_note->occurs_at_second= curr_row_second;
			curr_note->id_in_chart= static_cast<float>(curr_note_id);
			curr_note->id_in_column= static_cast<float>(column_ids[curr_note.Track()]);
			curr_note->row_id= static_cast<float>(curr_row_id);
			++curr_note_id;
			++column_ids[curr_note.Track()];
			notes_on_curr_row.push_back(&(*curr_note));
			if(curr_note->subType != TapNoteSubType_Invalid)
			{
				if(curr_note->subType > highest_subtype_on_row || highest_subtype_on_row == TapNoteSubType_Invalid)
				{
					highest_subtype_on_row= curr_note->subType;
				}
			}
			if(curr_note->type == TapNoteType_HoldHead)
			{
				curr_note->end_second= timing_data->GetElapsedTimeFromBeat(NoteRowToBeat(curr_row + curr_note->iDuration));
			}
		}
		++curr_note;
	}
	for(auto&& note : notes_on_curr_row)
	{
		note->highest_subtype_on_row= highest_subtype_on_row;
	}

    // finish? TODO: make internal counter
	td->ReleaseLookup();
}

// @description only use for VOS format!
void NoteData::FillBeatData(const TimingData& td)
{

}


/*
* metadata utilities
*/
int NoteData::GetTrackCount()
{
    return m_Tracks.size();
}

void NoteData::SetTrackCount(int iTrackCnt)
{
    ASSERT(iTrackCnt > 0);
    m_Tracks.resize( iTrackCnt );
}

int NoteData::GetNoteCount()
{
    int i=0;
    int iNoteCount=0;
    for (i=0; i<GetTrackCount(); i++)
        iNoteCount += GetNoteCount(i);
    return iNoteCount;
}

int NoteData::GetNoteCount(int iTrackNum)
{
    return m_Tracks[iTrackNum].size();
}

// @description 
int NoteData::GetLastNoteRow()
{
    return 0;
}

int NoteData::GetLastNoteTime()
{
    return 0;
}

bool NoteData::IsHoldNoteAtRow(int track, int row)
{
    return 0;
}

NoteType NoteData::GetNoteTypeAtRow(int track, int row)
{
    return 0;
}

Note* NoteData::GetNoteAtRow(int track, int row)
{
    return 0;
}

bool NoteData::IsRangeEmpty(int track, int iStartRow, int iEndRow)
{
    return 0;
}

bool NoteData::IsRowEmpty(int row)
{
    return 0;
}

bool NoteData::IsTrackEmpty(int track)
{
    return 0;
}

bool NoteData::IsEmpty()
{
    return 0;
}

void CopyRange(int rowFromBegin, int rowFromLength, int rowToBegin);